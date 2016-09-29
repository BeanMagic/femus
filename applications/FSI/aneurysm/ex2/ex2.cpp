#include "MultiLevelProblem.hpp"
#include "NumericVector.hpp"
#include "Fluid.hpp"
#include "Solid.hpp"
#include "Parameter.hpp"
#include "FemusInit.hpp"
#include "SparseMatrix.hpp"
#include "FElemTypeEnum.hpp"
#include "Files.hpp"
#include "MonolithicFSINonLinearImplicitSystem.hpp"
#include "../include/FSISteadyStateAssembly.hpp"
#include "VTKWriter.hpp"

double scale = 1000.;

using namespace std;
using namespace femus;


bool SetBoundaryCondition(const std::vector < double >& x, const char name[],
                          double &value, const int facename, const double time);

//------------------------------------------------------------------------------------------------------------------

int main(int argc, char **args) {

  // ******* Init Petsc-MPI communicator *******
  FemusInit mpinit(argc, args, MPI_COMM_WORLD);


  bool dimension2D = false;

  //Files files;
  //files.CheckIODirectories();
  //files.RedirectCout();

  // ******* Extract the problem dimension and simulation identifier based on the inline input *******


  // ******* Extract the mesh.neu file name based on the simulation identifier *******
  std::string infile = "./input/aneurysm_Sara_5.neu";

  // ******* Set physics parameters *******
  double Lref, Uref, rhof, muf, rhos, ni, E;

  Lref = 1.;
  Uref = 1.;

  rhof = 100.;
  muf = 1.;
  rhos = 800;
  ni = 0.5;
  E = 1800000;

  Parameter par(Lref, Uref);

  // Generate Solid Object
  Solid solid;
  solid = Solid(par, E, ni, rhos, "Mooney-Rivlin");

  cout << "Solid properties: " << endl;
  cout << solid << endl;

  // Generate Fluid Object
  Fluid fluid(par, muf, rhof, "Newtonian");
  cout << "Fluid properties: " << endl;
  cout << fluid << endl;

  // ******* Init multilevel mesh from mesh.neu file *******
  unsigned short numberOfUniformRefinedMeshes, numberOfAMRLevels;

  numberOfUniformRefinedMeshes = 2;
  numberOfAMRLevels = 0;

  std::cout << 0 << std::endl;

  MultiLevelMesh ml_msh(numberOfUniformRefinedMeshes + numberOfAMRLevels, numberOfUniformRefinedMeshes,
                        infile.c_str(), "fifth", Lref, NULL);

  //ml_msh.EraseCoarseLevels(numberOfUniformRefinedMeshes - 1);

  ml_msh.PrintInfo();

  // mark Solid nodes
  ml_msh.MarkStructureNode();


  std::cout << 1 << std::endl;


  // ******* Init multilevel solution ******
  MultiLevelSolution ml_sol(&ml_msh);

  // ******* Add solution variables to multilevel solution and pair them *******
  ml_sol.AddSolution("DX", LAGRANGE, SECOND, 1);
  ml_sol.AddSolution("DY", LAGRANGE, SECOND, 1);
  if(!dimension2D) ml_sol.AddSolution("DZ", LAGRANGE, SECOND, 1);

  ml_sol.AddSolution("U", LAGRANGE, SECOND, 1);
  ml_sol.AddSolution("V", LAGRANGE, SECOND, 1);
  if(!dimension2D) ml_sol.AddSolution("W", LAGRANGE, SECOND, 1);

  // Pair each velocity variable with the corresponding displacement variable
  ml_sol.PairSolution("U", "DX"); // Add this line
  ml_sol.PairSolution("V", "DY"); // Add this line
  if(!dimension2D) ml_sol.PairSolution("W", "DZ"); // Add this line

  // Since the Pressure is a Lagrange multiplier it is used as an implicit variable
  ml_sol.AddSolution("P", DISCONTINOUS_POLYNOMIAL, FIRST, 1);
  ml_sol.AssociatePropertyToSolution("P", "Pressure", false); // Add this line

  // ******* Initialize solution *******
  ml_sol.Initialize("All");
  ml_sol.AttachSetBoundaryConditionFunction(SetBoundaryCondition);

  // ******* Set boundary conditions *******
  ml_sol.GenerateBdc("DX", "Steady");
  ml_sol.GenerateBdc("DY", "Steady");
  if(!dimension2D) ml_sol.GenerateBdc("DZ", "Steady");
  ml_sol.GenerateBdc("U", "Steady");
  ml_sol.GenerateBdc("V", "Steady");
  if(!dimension2D) ml_sol.GenerateBdc("W", "Steady");
  ml_sol.GenerateBdc("P", "Steady");


  // ******* Define the FSI Multilevel Problem *******

  std::cout << 2 << std::endl;
  MultiLevelProblem ml_prob(&ml_sol);
  // Add fluid object
  ml_prob.parameters.set<Fluid>("Fluid") = fluid;
  // Add Solid Object
  ml_prob.parameters.set<Solid>("Solid") = solid;


  std::cout << 3 << std::endl;

  // ******* Add FSI system to the MultiLevel problem *******
  MonolithicFSINonLinearImplicitSystem & system = ml_prob.add_system<MonolithicFSINonLinearImplicitSystem> ("Fluid-Structure-Interaction");
  system.AddSolutionToSystemPDE("DX");
  system.AddSolutionToSystemPDE("DY");
  if(!dimension2D) system.AddSolutionToSystemPDE("DZ");
  system.AddSolutionToSystemPDE("U");
  system.AddSolutionToSystemPDE("V");
  if(!dimension2D) system.AddSolutionToSystemPDE("W");
  system.AddSolutionToSystemPDE("P");

  // ******* System Fluid-Structure-Interaction Assembly *******
  system.SetAssembleFunction(FSISteadyStateAssembly);

  std::cout << 4 << std::endl;

  // ******* set MG-Solver *******
  system.SetMgType(F_CYCLE);

  std::cout << 5 << std::endl;

  system.SetNonLinearConvergenceTolerance(1.e-9);
  system.SetResidualUpdateConvergenceTolerance(1.e-15);
  system.SetMaxNumberOfNonLinearIterations(2);
  system.SetMaxNumberOfResidualUpdatesForNonlinearIteration(1);

  system.SetNumberPreSmoothingStep(0);
  system.SetNumberPostSmoothingStep(2);

  // ******* Set Preconditioner *******

  std::cout << 6 << std::endl;

  system.SetMgSmoother(ASM_SMOOTHER);

  system.init();

  std::cout << 7 << std::endl;

  // ******* Set Smoother *******
  system.SetSolverFineGrids(RICHARDSON);
  //system.SetSolverFineGrids(GMRES);

  system.SetPreconditionerFineGrids(ILU_PRECOND);

  system.SetTolerances(1.e-12, 1.e-20, 1.e+50, 20, 10);

  // ******* Add variables to be solved *******
  system.ClearVariablesToBeSolved();
  system.AddVariableToBeSolved("All");

  // ******* Set the last (1) variables in system (i.e. P) to be a schur variable *******
  system.SetNumberOfSchurVariables(1);

  // ******* Set block size for the ASM smoothers *******
  system.SetElementBlockNumber(2);

  // ******* For Gmres Preconditioner only *******
  //system.SetDirichletBCsHandling(ELIMINATION);

  // ******* Solve *******
  std::cout << std::endl;
  std::cout << " *********** Fluid-Structure-Interaction ************  " << std::endl;
  system.MGsolve();

  std::cout << 8 << std::endl;

  // ******* Print solution *******
  ml_sol.SetWriter(GMV);

  std::vector<std::string> mov_vars;
  mov_vars.push_back("DX");
  mov_vars.push_back("DY");
  mov_vars.push_back("DZ");
  ml_sol.GetWriter()->SetMovingMesh(mov_vars);

  std::vector<std::string> print_vars;
  print_vars.push_back("All");

  //ml_sol.GetWriter()->SetDebugOutput( true );
  //ml_sol.GetWriter()->ParallelWrite(DEFAULT_OUTPUTDIR,"biquadratic",print_vars);
  VTKWriter vtkIO(&ml_sol);
  vtkIO.SetDebugOutput(true);
  vtkIO.Write(DEFAULT_OUTPUTDIR, "biquadratic", print_vars);

  // ******* Clear all systems *******
  ml_prob.clear();
  return 0;
}


//---------------------------------------------------------------------------------------------------------------------





bool SetBoundaryCondition(const std::vector < double >& x, const char name[], double &value, const int facename, const double time) {
  bool test = 1; //dirichlet
  value = 0.;

  if(!strcmp(name, "U") || !strcmp(name, "V") || !strcmp(name, "W")) {

    if(1 == facename  || 2 == facename || 3 == facename) {
      test = 0;
      if(!strcmp(name, "U")) {
        value = 1.;
      }
      else if(7 == facename) {
        value = 0.;
      }
    }
  }

  else if(!strcmp(name, "P")) {
    test = 0;
    value = 0.;
  }

  else if(!strcmp(name, "DX") || !strcmp(name, "DY") || !strcmp(name, "DZ")) {
    if(7 == facename) {
      test = 0;
      value = 1.;
    }
  }
  
  return test;
  
}
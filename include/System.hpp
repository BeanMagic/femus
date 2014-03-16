/*=========================================================================

 Program: FEMUS
 Module: System
 Authors: Simone Bnà
 
 Copyright (c) FEMTTU
 All rights reserved. 

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __System_hpp__
#define __System_hpp__

//----------------------------------------------------------------------------
// includes :
//----------------------------------------------------------------------------
#include "NonLinearMultiLevelProblem.hpp"


//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------
class System;
class NonLinearMultiLevelProblem;
class String;


class System {

public:

  /** Constructor.  Optionally initializes required data structures. */
  System (NonLinearMultiLevelProblem& es, const std::string& name, const unsigned int number);

  /** destructor */
  virtual ~System();
  
  unsigned int number() const;
  
  const std::string & name() const;
  
  /** Associate the solution variables to the system PDE */
  void AddSolutionToSytemPDE(const char solname[]);
  
   /** Clear all the data structures associated with the system. */
  virtual void clear();
  
  /** Init the system PDE structures */
  virtual void init();

protected:
  
  /** Constant reference to the \p EquationSystems object used for the simulation. */
  NonLinearMultiLevelProblem& _equation_systems;
  
  vector <unsigned> _SolSystemPdeIndex;

private:
  
  /** A name associated with this system. */
  const std::string _sys_name;

  /** The number associated with this system */
  const unsigned int _sys_number;
  
};

// System inline methods
inline
const std::string & System::name() const
{
  return _sys_name;
}

inline
unsigned int System::number() const
{
  return _sys_number;
}

#endif
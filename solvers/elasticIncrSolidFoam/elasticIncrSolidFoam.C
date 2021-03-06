/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2004-2007 Hrvoje Jasak
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

Application
    elasticIncrSolidFoam

Description
    Transient/steady-state segregated finite-volume solver for small strain
    elastic solid bodies.

    Displacement Increment field DU is solved for using a incremental total
    Lagrangian approach,
    also generating the strain tensor field epsilon and stress tensor
    field sigma.

    With optional multi-material solid interface correction ensuring
    correct tractions on multi-material interfaces

Author
    Philip Cardiff
    multi-material by Tukovic et al. 2012

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "rheologyModel.H"
//#include "solidInterface.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
# include "setRootCase.H"

# include "createTime.H"

# include "createMesh.H"

# include "createFields.H"

# include "readDivDSigmaExpMethod.H"

  //# include "createSolidInterface.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

  Info<< "\nCalculating displacement field\n" << endl;

  while(runTime.loop())
    {
      Info<< "Time: " << runTime.timeName() << nl << endl;
      
#     include "readStressedFoamControls.H"

      int iCorr = 0;
      scalar initialResidual = 0;
      scalar relativeResidual = GREAT;
      //lduMatrix::solverPerformance solverPerf;
      solverPerformance solverPerf;
      lduMatrix::debug = 0;

      do
        {
	  DU.storePrevIter();

#         include "calculateDivDSigmaExp.H"

	  //- linear momentum equation
	  fvVectorMatrix DUEqn
            (
	     fvm::d2dt2(rho, DU)
	     ==
	     fvm::laplacian(2*muf + lambdaf, DU, "laplacian(DDU,DU)")
	     + divDSigmaExp
	     );

// 	  if(solidInterfaceCorr)
// 	    {
// 	      solidInterfacePtr->correct(DUEqn);
// 	    }

	  solverPerf = DUEqn.solve();

	  if(iCorr == 0)
	    {
	      initialResidual = solverPerf.initialResidual();
	    }
	  
	  DU.relax();

// 	  if(solidInterfaceCorr)
// 	    {
// 	      gradDU = solidInterfacePtr->grad(DU);
// 	    }
// 	  else
// 	    {
	      gradDU = fvc::grad(DU);
// 	    }

#         include "calculateRelativeResidual.H"
	  
	  Info << "\tTime " << runTime.value()
	       << ", Corrector " << iCorr
	       << ", Solving for " << DU.name()
	       << " using " << solverPerf.solverName()
	       << ", residual = " << solverPerf.initialResidual()
	       << ", relative residual = " << relativeResidual << endl;
        }
	while
	  (
	   //solverPerf.initialResidual() > convergenceTolerance
	   relativeResidual > convergenceTolerance
	   &&
	   ++iCorr < nCorr
	   );
	
      Info << nl << "Time " << runTime.value() << ", Solving for " << DU.name() 
	   << ", Initial residual = " << initialResidual 
	   << ", Final residual = " << solverPerf.initialResidual()
	   << ", Relative residual = " << relativeResidual
	   << ", No outer iterations " << iCorr
	   << nl << "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
	   << "  ClockTime = " << runTime.elapsedClockTime() << " s" 
	   << endl;
      
      lduMatrix::debug=0;

#     include "calculateDEpsilonDSigma.H"

      U += DU;
      sigma += DSigma;
      epsilon += DEpsilon;

#     include "writeFields.H"

#     include "calculateNetForces.H"

      Info<< "ExecutionTime = "
	  << runTime.elapsedCpuTime()
	  << " s\n\n" << endl;
    }
  
  Info<< "End\n" << endl;
  
  return(0);
}


// ************************************************************************* //

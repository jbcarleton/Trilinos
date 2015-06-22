// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
/*
 * MueLu_AggregationExportFactory_def.hpp
 *
 *  Created on: Feb 10, 2012
 *      Author: wiesner
 */

#ifndef MUELU_AGGREGATIONEXPORTFACTORY_DEF_HPP_
#define MUELU_AGGREGATIONEXPORTFACTORY_DEF_HPP_

#include <Xpetra_Matrix.hpp>
#include <Xpetra_CrsMatrixWrap.hpp>

#include "MueLu_AggregationExportFactory_decl.hpp"

#include "MueLu_Level.hpp"
#include "MueLu_Aggregates.hpp"
#include "MueLu_AmalgamationFactory.hpp"
#include "MueLu_AmalgamationInfo.hpp"
#include "MueLu_Monitor.hpp"
#include "MueLu_Utilities.hpp"
#include <vector>
#include <list> //singly-linked list
#include <string>
#include <stdexcept>

namespace MueLu {

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  RCP<const ParameterList> AggregationExportFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node>::GetValidParameterList() const {
    RCP<ParameterList> validParamList = rcp(new ParameterList());

    std::string output_msg = "Output filename template (%TIMESTEP is replaced by \'Output file: time step\' variable,"
        "%ITER is replaced by \'Output file: iter\' variable, %LEVELID is replaced level id, %PROCID is replaced by processor id)";
    std::string output_def = "aggs_level%LEVELID_proc%PROCID.out";

    validParamList->set< RCP<const FactoryBase> >("Aggregates",             Teuchos::null, "Generating factory for aggregates");
    validParamList->set< RCP<const FactoryBase> >("DofsPerNode",            Teuchos::null, "Generating factory for number of dofs per node");
    validParamList->set< RCP<const FactoryBase> >("UnAmalgamationInfo",     Teuchos::null, "Generating factory for amalgamation");
    validParamList->set< RCP<const FactoryBase> >("Coordinates",            Teuchos::null, "Transfer factory for coordinates");

    // CMS/BMK: Old style factory-only options.  Deprecate me.
    validParamList->set< std::string >           ("Output filename",           output_def, output_msg);
    validParamList->set< int >                   ("Output file: time step",             0, "time step variable for output file name");
    validParamList->set< int >                   ("Output file: iter",                  0, "nonlinear iteration variable for output file name");

    // New-style master list options
    validParamList->set< std::string >           ("aggregation: output filename",                    output_def, output_msg);
    validParamList->set< int >                   ("aggregation: output file time step",              0,          "time step variable for output file name");// Remove me?
    validParamList->set< int >                   ("aggregation: output file: iter",                  0,          "nonlinear iteration variable for output file name");//Remove me?
    validParamList->set<std::string>             ("aggregation: output file: agg style",             "",         "style of aggregate visualization for VTK output");
    return validParamList;
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  void AggregationExportFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node>::DeclareInput(Level &fineLevel, Level &coarseLevel) const {
    Input(fineLevel, "Aggregates");         //< factory which created aggregates
    Input(fineLevel, "DofsPerNode");        //< CoalesceAndDropFactory (needed for DofsPerNode variable)
    Input(fineLevel, "UnAmalgamationInfo"); //< AmalgamationFactory (needed for UnAmalgamationInfo variable)
    Input(fineLevel, "Coordinates");
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  void AggregationExportFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node>::Build(Level &fineLevel, Level &coarseLevel) const {
      std::cout << "\n\n\n\n\nInvoking AggregationExportFactory build!\n\n\n\n\n";
    const ParameterList & pL = GetParameterList();
      std::cout << "Master filename is: " << pL.get<std::string>("aggregation: output filename");
    FactoryMonitor m(*this, "AggregationExportFactory", coarseLevel);
    std::cout << "Fine level is " << fineLevel.GetLevelID() << " and coarse level is " << coarseLevel.GetLevelID() << std::endl;

    std::string masterFilename = pL.get<std::string>("aggregation: output filename");
    std::string outFile  = pL.get<std::string>("Output filename");
    //bool useVTK = !(strstr(masterFilename.c_str(), ".vt") == NULL);
      bool useVTK = true;
    //std::cout << masterFilename << std::endl;
    if(useVTK)
      outFile = masterFilename;
    Teuchos::RCP<Aggregates> aggregates      = Get< Teuchos::RCP<Aggregates> >(fineLevel,"Aggregates");
    LocalOrdinal DofsPerNode                 = Get< LocalOrdinal >            (fineLevel,"DofsPerNode");
    Teuchos::RCP<AmalgamationInfo> amalgInfo = Get< RCP<AmalgamationInfo> >   (fineLevel,"UnAmalgamationInfo");
    Teuchos::RCP<Xpetra::MultiVector<double, LocalOrdinal, GlobalOrdinal, Node>> coords = Get<RCP<Xpetra::MultiVector<double, LocalOrdinal, GlobalOrdinal, Node>>>(fineLevel, "Coordinates");
    
    GetOStream(Runtime0) << "AggregationExportFactory: DofsPerNode: " << DofsPerNode << std::endl;
    Teuchos::RCP<const Teuchos::Comm<int> > comm = aggregates->GetMap()->getComm();
    int numProcs = comm->getSize();
    int myRank   = comm->getRank();

    Teuchos::RCP<LocalOrdinalVector> vertex2AggId_vector = aggregates->GetVertex2AggId();
    Teuchos::RCP<LocalOrdinalVector> procWinner_vector   = aggregates->GetProcWinner();
    Teuchos::ArrayRCP<LocalOrdinal>  vertex2AggId        = aggregates->GetVertex2AggId()->getDataNonConst(0);
    Teuchos::ArrayRCP<LocalOrdinal>  procWinner          = aggregates->GetProcWinner()->getDataNonConst(0);

    // prepare for calculating global aggregate ids
    std::vector<GlobalOrdinal> numAggsGlobal (numProcs, 0);
    std::vector<GlobalOrdinal> numAggsLocal  (numProcs, 0);
    std::vector<GlobalOrdinal> minGlobalAggId(numProcs, 0);

    numAggsLocal[myRank] = aggregates->GetNumAggregates();
    Teuchos::reduceAll(*comm, Teuchos::REDUCE_SUM, numProcs, &numAggsLocal[0], &numAggsGlobal[0]);
    for (int i = 1; i < Teuchos::as<int>(numAggsGlobal.size()); ++i) {
      numAggsGlobal [i] += numAggsGlobal[i-1];
      minGlobalAggId[i]  = numAggsGlobal[i-1];
    }

    ArrayRCP<LO>            aggStart;
    ArrayRCP<GlobalOrdinal> aggToRowMap;
    amalgInfo->UnamalgamateAggregates(*aggregates, aggStart, aggToRowMap);

    // write to file
    //std::string outFile = outputFileName_;
    int timeStep, iter;
    if(!useVTK)
    {
      timeStep = pL.get< int >      ("Output file: time step");
      iter     = pL.get< int >      ("Output file: iter");
      outFile = replaceAll(outFile, "%LEVELID",  toString(fineLevel.GetLevelID()));
      outFile = replaceAll(outFile, "%PROCID",   toString(myRank));
      outFile = replaceAll(outFile, "%TIMESTEP", toString(timeStep));
      outFile = replaceAll(outFile, "%ITER",     toString(iter));
    }
    GetOStream(Runtime0) << "AggregationExportFactory: outputfile \"" << outFile << "\"" << std::endl;
    //does the user want a widely compatible .vtp file (xml formatted data) to visualize aggregates in ParaView?
    //If filename ends in .vtp (which it will have to be for an unstructured 'PolyData' VTK file), do that
    //This is the filename that VTK users will set, so check that for a valid filename

    std::ofstream fout(outFile);
    GO numAggs = aggregates->GetNumAggregates();
    if(!useVTK)
    {
      GO indexBase = aggregates->GetMap()->getIndexBase(); // extract indexBase from overlapping map within aggregates structure. The indexBase is constant throughout the whole simulation (either 0 = C++ or 1 = Fortran)
      GO offset    = amalgInfo->GlobalOffset();            // extract offset for global dof ids
      std::vector<GlobalOrdinal> nodeIds;
      for (int i = 0; i < numAggs; ++i) {
        fout << "Agg " << minGlobalAggId[myRank] + i << " Proc " << myRank << ":";
  
        // TODO: Use k+=DofsPerNode instead of ++k and get rid of std::unique call afterwards
        for (int k = aggStart[i]; k < aggStart[i+1]; ++k) {
          nodeIds.push_back((aggToRowMap[k] - offset - indexBase) / DofsPerNode + indexBase);       
        }

        // remove duplicate entries from nodeids
        std::sort(nodeIds.begin(), nodeIds.end());
        typename std::vector<GlobalOrdinal>::iterator endLocation = std::unique(nodeIds.begin(), nodeIds.end());
        nodeIds.erase(endLocation, nodeIds.end());

        // print out nodeids 
        for(typename std::vector<GlobalOrdinal>::iterator printIt = nodeIds.begin(); printIt != nodeIds.end(); printIt++)
          fout << " " << *printIt;
        nodeIds.clear();
        fout << std::endl;
      }
    }
    else
    {
      //Note: For now, this is only gonna work with real scalars.
      using namespace std;
      if(sizeof(Scalar) != sizeof(double))
        throw runtime_error("Complex scalars not supported in aggregate visualization.");
      //fetch coordinate arrary, every style needs them
/*
      Teuchos::RCP<Xpetra::MultiVector<double, LocalOrdinal, GlobalOrdinal, Node>> coords = Teuchos::null;
      try
      {
        coords = fineLevel.Get<Teuchos::RCP<Xpetra::MultiVector<double, LocalOrdinal, GlobalOrdinal, Node>>>("Coordinates");
      }
      catch(exception& e)
      {
        cout << "Error: Creating VTK aggregate visualization requires coordinates array in the fine level." << endl;
        fout.close();
        return;
      }
*/
      int numNodes = coords->getGlobalLength();
      int dims = coords->getNumVectors();  //2D or 3D?
      //get access to the coord data
      Teuchos::ArrayRCP<const double> xCoords = coords->getData(0);
      Teuchos::ArrayRCP<const double> yCoords = coords->getData(1);
      Teuchos::ArrayRCP<const double> zCoords = Teuchos::null;
      const Teuchos::RCP<Xpetra::Vector<LocalOrdinal, LocalOrdinal, GlobalOrdinal, Node>> procWinnersVec = aggregates->GetProcWinner();
      Teuchos::ArrayRCP<const LocalOrdinal> procWinners = procWinnersVec->getData(0);
      if(dims == 3)
        zCoords = coords->getData(2);
      //Get the sizes of the aggregates to speed up grabbing node IDs
      Teuchos::ArrayRCP<GlobalOrdinal> aggSizes = aggregates->ComputeAggregateSizes();
      string aggStyle = "Point Cloud";
      try
      {
        aggStyle = pL.get<string>(string("Output file: agg style")); //Let "Point Cloud" be the default style
      }
      catch(exception& e) {}
      fout << "<!--Aggregates Visualization-->" << endl;
      string indent = "";
      if(aggStyle == "Point Cloud")
      {
        fout << "<!--Point Cloud-->" << std::endl;
        fout << "<VTKFile type=\"PolyData\" byte_order=\"LittleEndian\">" << std::endl;
        fout << "  <PolyData>" << std::endl;
        //Number of points in each "piece" will be the number of nodes in each aggregate
        fout << "    <Piece NumberOfPoints=\"" << numNodes << "\" NumberOfVerts=\"0\" NumberOfLines=\"0\" NumberOfStrips=\"0\" NumberOfPolys=\"0\">" << endl;
        fout << "      <PointData Scalars=\"Node\" \"Aggregate\" \"Processor\">" << endl;
        indent = "          ";
        fout << "        <DataArray type=\"Int32\" Name=\"Node\" format=\"ascii\"" << endl;
        fout << indent;
        for(int node = 0; node < numNodes; node++)
        {
          fout << node << " ";
          if(node % 8 == 7)
            fout << endl << indent;
        }
        fout << endl;
        fout << "        </DataArray>" << endl;
        fout << "        <DataArray type=\"Int32\" Name=\"Aggregate\" format=\"ascii\"" << endl;
        fout << indent;
        for(int node = 0; node < numNodes; node++)
        {
          fout << vertex2AggId[node] << " ";
          if(node % 8 == 7)
            fout << endl << indent;
        }
        fout << endl;
        fout << "        </DataArray>" << endl;
        fout << "        <DataArray type=\"Int32\" Name=\"Processor\" format=\"ascii\"" << endl;
        fout << indent;
        for(int node = 0; node < numNodes; node++)
        {
          fout << procWinners[node] << " ";
          if(node % 8 == 7)
            fout << endl << indent;
        }
        fout << endl;
        fout << "        </DataArray>" << endl;
        fout << "      </PointData>" << endl;
        //Write the point coordinates
        fout << "      <Points>" << endl;
        fout << "        <Coordinates>" << endl;
        indent = "          ";
        fout << "          <DataArray type=\"Float64\" format=\"ascii\">" << endl;  //x coordinates
        fout << indent;
        for(int node = 0; node < numNodes; node++)
        {
          fout << xCoords[node] << " ";
          if(node % 8 == 7)
            fout << endl << indent;
        }
        fout << endl;
        fout << "          </DataArray>" << endl;
        fout << "          <DataArray type=\"Float64\" format=\"ascii\">" << endl;
        fout << indent;
        for(int node = 0; node < numNodes; node++)
        {
          fout << yCoords[node] << " ";
          if(node % 8 == 7)
            fout << endl << indent;
        }
        fout << endl;
        fout << "          </DataArray>" << endl;
        if(dims == 3)
        {
          fout << "          <DataArray type=\"Float64\" format=\"ascii\">" << endl;
          for(int node = 0; node < numNodes; node++)
          {
            fout << zCoords[node] << " ";
            if(node % 8 == 7)
              fout << endl << indent;
          }
          fout << endl;
          fout << "          </DataArray>" << endl;
        }
        fout << "        </Coordinates>" << endl;
        fout << "      </Points>" << endl;
        fout << "    </Piece>" << endl << endl;
        fout << "  </PolyData>" << std::endl;
        fout << "</VTKFile>" << std::endl;
      }
      else if(aggStyle == "Jacks")
      {
        fout << "<!--Jacks-->" << endl;
        fout << "<VTKFile type=\"PolyData\" byte_order=\"LittleEndian\">" << endl;
        int totalEdges = 0;//VTK needs to know this right away so compute it
        for(int i = 0; i < numAggs; i++)
        {
          totalEdges += aggSizes[i] - 1;
        }
        fout << "  <PolyData>" << endl;
        fout << "    <Piece NumberOfPoints=\"" << numNodes << "NumberOfVerts=\"0\" NumberOfLines=\"" << totalEdges << "\" NumberOfStrips=\"0\" NumberOfPolys=\"0\">" << endl;
        fout << "      <PointData Scalars=\"Node\" \"Aggregate\" \"Processor\">" << endl;
        indent = "          ";
        fout << "        <DataArray type=\"Int32\" Name=\"Node\" format=\"ascii\"" << endl;
        fout << indent;
        for(int node = 0; node < numNodes; node++)
        {
          fout << node << " ";
          if(node % 8 == 7)
            fout << endl << indent;
        }
        fout << endl;
        fout << "        </DataArray>" << endl;
        fout << "        <DataArray type=\"Int32\" Name=\"Aggregate\" format=\"ascii\"" << endl;
        fout << indent;
        for(int node = 0; node < numNodes; node++)
        {
          fout << vertex2AggId[node] << " ";
          if(node % 8 == 7)
            fout << endl << indent;
        }
        fout << endl;
        fout << "        </DataArray>" << endl;
        fout << "        <DataArray type=\"Int32\" Name=\"Processor\" format=\"ascii\"" << endl;
        fout << indent;
        for(int node = 0; node < numNodes; node++)
        {
          fout << procWinners[node] << " ";
          if(node % 8 == 7)
            fout << endl << indent;
        }
        fout << endl;
        fout << "        </DataArray>" << endl;
        fout << "      </PointData>" << endl;
        fout << "      <Points>" << endl;
        fout << "        <Coordinates>" << endl;
        indent = "            ";
        fout << "          <DataArray type=\"Float64\" format=\"ascii\">" << endl;
        fout << indent;
        for(int i = 0; i < numNodes; i++)
        {
          fout << xCoords[i] << " ";
          if(i % 8 == 7)
            fout << endl << indent;
        }
        fout << endl;
        fout << "          </DataArray>" << endl;
        fout << "          <DataArray type=\"Float64\" format=\"ascii\">" << endl;
        fout << indent;
        for(int i = 0; i < numNodes; i++)
        {
          fout << yCoords[i] << " ";
          if(i % 8 == 7)
            fout << endl << indent;
        }
        fout << endl;
        fout << "          </DataArray>" << endl;
        if(dims == 3)
        {
          fout << "          <DataArray type=\"Float64\" format=\"ascii\">" << endl;
          fout << indent;
          for(int i = 0; i < numNodes; i++)
          {
            fout << zCoords[i] << " ";
            if(i % 8 == 7)
              fout << endl << indent;
          }
          fout << endl;
          fout << "          </DataArray>" << endl;
        }
        fout << "        <Coordinates>" << endl;
        fout << "      </Points>" << endl;
        fout << "      <Lines>" << endl;
        //Form list of node pairs
        vector<int> connections;
        //For each aggregate, find the root node then connect it with the other nodes in the aggregate
        //Doesn't matter the order, as long as all edges are found.
        int root = 0;
        for(int i = 0; i < numAggs; i++)
        {
          while(!aggregates->IsRoot(root))
            root++;
          int numInAggFound = 0;
          for(int j = 0; j < numNodes; j++)
          {
            if(vertex2AggId[root] == vertex2AggId[j])
            {
              connections.push_back(root);
              connections.push_back(j);
              numInAggFound++;
                if(numInAggFound == aggSizes[vertex2AggId[root]]) //don't spend more time looking if done with that root
                break;
            }
          }
        }
        //List pairs of nodes to connect with lines
        fout << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">" << endl;
        indent = "          ";
        fout << indent;
        for(int i = 0; i < int(connections.size()); i++)
        {
          fout << connections[i] << " ";
          if(i % 8 == 7)
            fout << endl << indent;
        }
        fout << endl;
        fout << "        </DataArray>" << endl;
        fout << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">" << endl;
        fout << indent;
        for(int i = 0; i < int(connections.size()); i += 2)
        {
          fout << i << " ";
          if(i % 16 == 14)
            fout << endl << indent;
        }
        fout << endl;
        fout << "        </DataArray>" << endl;
        fout << "      </Lines>" << endl;
        fout << "    </Piece>" << endl;
        fout << "  </PolyData>" << endl;
        fout << "</VTKFile>" << endl;
      }
      else if(aggStyle == "Jacks++")
      {
        //TODO: will Jacks++ even be implemented in this file? Might have to be externel post-processing with Aggregates, Coordinates and Graph. Matlab?
      }
      else if(aggStyle == "Convex Hulls")
      {
        if(dims == 2)
        {
          vector<int> hullPoints;    //node IDs of the nodes which are vertices of the convex hulls, in order of aggregates and then clockwise
          vector<int> hullSizes;     //# of vertices in the hulls representing each aggregate.
          for(int agg = 0; agg < numAggs; agg++)
          {
            vector<int> aggNodes(aggSizes[agg]);  //First need to get all the nodes in the agg. Already know its size.
            for(int i = 0; i < numNodes; i++)
            {
              if(vertex2AggId[i] == agg)
                aggNodes.push_back(i);
              if(aggNodes.size() == aggSizes[agg])
                break;
            }
            //get first node in the hull
            int firstNode = aggNodes[0];
            for(int i = 1; i < int(aggNodes.size()); i++)
            {
              if(xCoords[aggNodes[i]] < xCoords[aggNodes[firstNode]])
                firstNode = aggNodes[i];
            }
            //x coordinate of firstNode is the minimum x. If multiple points with minimum x, find the one of those with minimum y.
            for(int i = 0; i < int(aggNodes.size()); i++)
            {
              if(xCoords[aggNodes[i]] == xCoords[firstNode] && yCoords[aggNodes[i]] < yCoords[firstNode])
                  firstNode = aggNodes[i];
            }
            //firstNode is now guaranteed to be in the hull
            hullPoints.push_back(firstNode);
            list<int> candidates;
            for(int i = 0; i < int(aggNodes.size()); i++)
            {
              if(aggNodes[i] != firstNode)
                candidates.push_back(aggNodes[i]);
            }
            //as points are added to 'points' of the hull, they are removed from a copy of aggNodes to speed up searching
            //Do Jarvis march.
            list<int>::iterator leftMost;
            int thisHullSize = 0;
            while(true)
            {
              leftMost = candidates.begin();
              int basePoint = hullPoints[hullPoints.size() - 1]; //point that is tail of vector toward leftMost
              list<int>::iterator it = leftMost;
              it++;
              for(; it != candidates.end(); it++)
              {
                //is point *it 'in front' or 'left of' the sweeping line? (dot prod)
                double dotRes = -(yCoords[*leftMost] - yCoords[basePoint]) * (xCoords[*it] - xCoords[basePoint]) + (xCoords[*leftMost] - xCoords[basePoint]) * (yCoords[*it] - yCoords[basePoint]);
                if(dotRes > 0)
                  leftMost = it;
              }
               //now leftMost has the next point in the hull
              //if that next point is the same as firstNode, we are done with this hull so break.
              if(*leftMost == firstNode)
                break;
              hullPoints.push_back(*leftMost);
              candidates.erase(leftMost);
              thisHullSize++;
            }
            hullSizes.push_back(thisHullSize);
          }
          //now have all the nodes that are hull vertices and the sizes of the hulls
          //write the PolyData and piece headers b/c now know how many elements will be in the piece
          fout << "<VTKFile type=\"PolyData\" byte_order=\"LittleEndian\">" << endl;
          fout << "  <PolyData>" << endl;
          fout << "    <Piece NumberOfPoints=\"" << hullPoints.size() << "\" NumberOfVerts=\"0\" NumberOfLines=\"" << hullPoints.size() << "\" NumberOfStrips=\"0\" NumberOfPolys=\"0\">" << endl;
          indent = "          ";
          fout << "      <PointData Scalars=\"Node\" \"Aggregate\" \"Processor\">" << endl;
          fout << "        <DataArray type=\"Int32\" Name=\"Node\" format=\"ascii\">" << endl;
          fout << indent;
          for(int i = 0; i < int(hullPoints.size()); i++)
          {
            fout << hullPoints[i] << " ";
            if(i % 8 == 7)
              fout << endl << indent;
          }
          fout << endl;
          fout << "        </DataArray>" << endl;
          fout << "        <DataArray type=\"Int32\" Name=\"Aggregate\" format=\"ascii\">" << endl;
          fout << indent;
          for(int i = 0; i < int(hullPoints.size()); i++)
          {
            fout << vertex2AggId[hullPoints[i]] << " ";
            if(i % 8 == 7)
              fout << endl << indent;
          }
          fout << endl;
          fout << "        </DataArray>" << endl;
          fout << "        <DataArray type=\"Int32\" Name=\"Aggregate\" format=\"ascii\">" << endl;
          fout << indent;
          for(int i = 0; i < int(hullPoints.size()); i++)
          {
            fout << procWinners[hullPoints[i]] << " ";
            if(i % 8 == 7)
              fout << endl << indent;
          }
          fout << endl;
          fout << "        </DataArray>" << endl;
          fout << "      </PointData>" << endl;
          //write out point coordinates
          fout << "      <Points>" << endl;
          fout << "        <Coordinates>" << endl;
          fout << "          <DataArray type=\"Float64\""; 
          fout << "        </Coordinates>" << endl;
          fout << "      </Points>" << endl;
          fout << "    </Piece>" << endl;
          fout << "  </PolyData>" << endl;
          fout << "</VTKFile>" << endl;
        }
        else if(dims == 3)
        {

        }
      }
      else if(aggStyle == "Alpha Hulls")
      {
        
      }
      else
      {
        fout << "<!-- Error: \'" << aggStyle << "\' is not a valid aggregate rendering style.-->" << std::endl;
        fout << "<!-- Try one of \'Point Cloud\', \'Jacks\', \'Jacks++\', \'Convex Hulls\', or \'Alpha Hulls\'.-->" << std::endl;
      }
    }
    fout.close();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  std::string AggregationExportFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node>::replaceAll(std::string result, const std::string& replaceWhat, const std::string& replaceWithWhat) const {
    while(1) {
      const int pos = result.find(replaceWhat);
      if (pos == -1)
        break;
      result.replace(pos, replaceWhat.size(), replaceWithWhat);
    }
    return result;
  }

} // namespace MueLu

#endif /* MUELU_AGGREGATIONEXPORTFACTORY_DEF_HPP_ */

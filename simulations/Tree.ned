//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

// Created: 2021-1-14 for project ds.base

package dsbase.simulations;

import dsbase.INode;

import dsbase.Edge;

   

//
// A generated network with k-ary tree topology.
//
network Tree
{
    parameters:
        string kind;
    submodules:
        node[14] : <kind> like INode;
    connections:
        node[0].port++  <--> Edge{weight = 10;} <--> node[8].port++;
        node[1].port++  <--> Edge{weight = 10;} <--> node[8].port++;
        node[2].port++  <--> Edge{weight = 10;} <--> node[9].port++;
        node[3].port++  <--> Edge{weight = 10;} <--> node[9].port++;
        node[4].port++  <--> Edge{weight = 10;} <--> node[10].port++;
        node[5].port++  <--> Edge{weight = 10;} <--> node[10].port++;
        node[6].port++  <--> Edge{weight = 10;} <--> node[11].port++;
        node[7].port++  <--> Edge{weight = 10;} <--> node[11].port++;
        node[8].port++  <--> Edge{weight = 10;} <--> node[12].port++;
        node[9].port++  <--> Edge{weight = 10;} <--> node[12].port++;
        node[10].port++ <--> Edge{weight = 10;} <--> node[13].port++;
        node[11].port++ <--> Edge{weight = 10;} <--> node[13].port++;
        node[12].port++ <--> Edge{weight = 1;} <--> node[13].port++;
}

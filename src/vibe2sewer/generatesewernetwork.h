/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of VIBe2
 *
 * Copyright (C) 2011  Christian Urich

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "compilersettings.h"

#ifndef GENERATESEWERNETWORK_H
#define GENERATESEWERNETWORK_H

#include "module.h"
#include <vector>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
namespace   ublas = boost::numeric::ublas;

using namespace vibens;
class VIBE_HELPER_DLL_EXPORT GenerateSewerNetwork : public  Module {
VIBe_DECLARE_NODE (GenerateSewerNetwork)
public:
    class Pos {
    public:
    long x;
    long y;
    double z;
    double h;
    Pos(long x, long y) {this->x = x; this->y = y;}
    Pos() {};
};
public:
class Agent {

public:
    Pos StartPos;
    std::vector<Pos> path;
    Pos currentPos;
    virtual void run();
    bool alive;
    bool successful;
    RasterData * Topology;
    RasterData * MarkPath;
    RasterData * ConnectivityField;    
    RasterData * Goals;
    RasterData * ForbiddenAreas;
    long steps;
    double AttractionTopology;
    double AttractionConnectivity;
    double Hmin;
    Agent(Pos);
protected:
    ublas::vector<double> neigh;
    ublas::vector<double> decissionVector;
    ublas::vector<double> ProbabilityVector;


};


private:
RasterData * Topology;
RasterData * ConnectivityField;
RasterData * ConnectivityField_in;
VectorData * StartPoints;
RasterData * Path;
RasterData * Goals;
RasterData * ForbiddenAreas;
int ConnectivityWidth;
double AttractionTopology;
double AttractionConnectivity;
long steps;
double Hmin;
std::string IdentifierStartPoins;

public:
GenerateSewerNetwork();
void run();

static void MarkPathWithField(const std::vector<Pos> & path, RasterData * ConnectivityField, int ConnectivityWidth);
static void addRadiusValueADD(int x, int y, RasterData * layer, int rmax, double value);
static void addRadiusValue(int x, int y, RasterData * layer, int rmax, double value);
static int indexOfMinValue(const ublas::vector<double> & vec);
};

#endif // GENERATESEWERNETWORK_H

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
#include "generatesewernetwork.h"

#include "csg_s_operations.h"


VIBe_DECLARE_NODE_NAME(GenerateSewerNetwork, Sewer)

GenerateSewerNetwork::Agent::Agent(Pos StartPos) {
    this->successful = false;
    this->alive = true;
    this->currentPos.x = StartPos.x;
    this->currentPos.y = StartPos.y;
    this->currentPos.z = 0;
    this->currentPos.h = 3;
    this->neigh = ublas::vector<double>(9);
    this->decissionVector = ublas::vector<double>(9);
    this->ProbabilityVector = ublas::vector<double>(9);

}

void GenerateSewerNetwork::Agent::run() {
    this->path.clear();
    for (int i = 0; i < this->steps; i++) {
        this->currentPos.z = this->Topology->getValue(currentPos.x, currentPos.y);
        double Hcurrent = this->currentPos.z;
        double hcurrent = this->currentPos.h;
        this->path.push_back(currentPos);

        //Influence Topology
        Topology->getMoorNeighbourhood(neigh, currentPos.x,currentPos.y);
        double currentHeight = neigh[4];
        int index = GenerateSewerNetwork::indexOfMinValue(neigh);
        for (int i = 0; i < 9; i++) {
            if (currentHeight + (this->Hmin - this->currentPos.h) >= neigh[i]) {
                decissionVector[i] = 1*this->AttractionTopology;

            } else {
                decissionVector[i] = 0;
            }
            if (index == i )
                decissionVector[i] += 1*this->AttractionTopology;
        }

        //Influence Topology
        ConnectivityField->getMoorNeighbourhood(neigh, currentPos.x,currentPos.y);
        currentHeight = neigh[4];
        index = GenerateSewerNetwork::indexOfMinValue(neigh);
        for (int i = 0; i < 9; i++) {
            if (currentHeight > neigh[i]) {
                decissionVector[i] += 1*this->AttractionConnectivity;                
            } else {
                decissionVector[i] += 0;
            }
            if (index == i)
                decissionVector[i] += 5*this->AttractionConnectivity;

        }


        //Forbidden Areas
        ForbiddenAreas->getMoorNeighbourhood(neigh, currentPos.x,currentPos.y);
        for (int i = 0; i < 9; i++) {
            decissionVector[i]*=neigh[i];
        }



        ProbabilityVector = decissionVector / ublas::sum(decissionVector) * 100;

        int ra = rand()%100;
        // Logger(vibens::Standard) << ra;
        double prob = 0;
        int direction = -1;
        for (int i = 0; i < 9; i++) {
            prob += ProbabilityVector[i];

            if (ra <= prob) {
                direction = i;
                break;
            }
        }
        this->currentPos.x+=csg_s::csg_s_operations::returnPositionX(direction);
        this->currentPos.y+=csg_s::csg_s_operations::returnPositionY(direction);
        double deltaH = Hcurrent - this->Topology->getValue(currentPos.x, currentPos.y);
        if (deltaH > 0) {
            currentPos.h = hcurrent - deltaH;
            if (currentPos.h < 3) {
                currentPos.h = 3;
            }
        } else {
            currentPos.h=hcurrent-deltaH;
        }
#

        MarkPath->setValue(currentPos.x, currentPos.y, 1);

        if (currentPos.x < 0 || currentPos.y < 0 || currentPos.x > Topology->getWidth()-2 || currentPos.y >  Topology->getHeight()-2) {
            this->alive = false;
            //this->successful = true;
            //this->path.push_back(currentPos);
            //Logger(vibens::Debug) << "Successful";
            break;


        }

        //Check current Pos is < 3 to secure connections
        if (Goals->getValue(currentPos.x, currentPos.y ) > 0 && currentPos.h < 3.1) {
            this->alive = false;
            this->successful = true;
            this->path.push_back(currentPos);
            break;
        }


    }

    this->alive = false;
}
GenerateSewerNetwork::GenerateSewerNetwork()
{
    this->ConnectivityWidth = 9;
    this->AttractionTopology = 1;
    this->AttractionConnectivity = 1;
    this->IdentifierStartPoins = "";
    this->steps = 1000;
    this->Hmin = 8;
    this->addParameter("Topology", VIBe2::RASTERDATA_IN, &this->Topology);
    this->addParameter("StartPoints", VIBe2::VECTORDATA_IN, &this->StartPoints);
    this->addParameter("IdentifierStartPoins", VIBe2::STRING, &this->IdentifierStartPoins);
    this->addParameter("Path", VIBe2::RASTERDATA_OUT, &this->Path);
    this->addParameter("MaxDeph", VIBe2::DOUBLE, &this->Hmin);
    this->addParameter("ConnectivityField", VIBe2::RASTERDATA_OUT, &this->ConnectivityField);
    this->addParameter("ConnectivityField_in", VIBe2::RASTERDATA_IN, &this->ConnectivityField_in);
    this->addParameter("ConnectivityWidth", VIBe2::LONG, & this->ConnectivityWidth);
    this->addParameter("AttractionTopology", VIBe2::DOUBLE, & this->AttractionTopology);
    this->addParameter("AttractionConnectivity", VIBe2::DOUBLE, & this->AttractionConnectivity);
    this->addParameter("Goals", VIBe2::RASTERDATA_IN, & this->Goals);
    this->addParameter("Steps", VIBe2::LONG, & this->steps);
    this->addParameter("ForbiddenAreas", VIBe2::RASTERDATA_IN, & this->ForbiddenAreas);


}
void GenerateSewerNetwork::addRadiusValueADD(int x, int y, RasterData *layer, int rmax, double value) {
    int level = rmax;
    if (rmax > 500) {
        rmax = 500;
    }
    int i = x - rmax;
    int j = y - rmax;

    if (i < 0) i = 0;
    if (j < 0) j = 0;

    int imax = 10;
    int jmax = 10;
    int k = 0;
    double z = rmax*rmax;
    for (i; i < rmax+x;  i++) {
        j = y - rmax;

        for ( j;  j < rmax+y; j++) {
            double r = double((i-x)*(i-x) + (j-y)*(j-y));
            double val = ((r-z)/10.)*(value);
            if (layer->getValue(i,j) > val )
                layer->setValue(i,j, val);
        }
    }
}
void GenerateSewerNetwork::addRadiusValue(int x, int y, RasterData * layer, int rmax, double value) {



        int level = rmax;
        if (rmax > 500) {
                rmax = 500;
        }
        int i = x - rmax;
        int j = y - rmax;

        if (i < 0) i = 0;
        if (j < 0) j = 0;

        int imax = 10;
        int jmax = 10;
        int k = 0;

        for (i; i < rmax+x;  i++) {
                j = y - rmax;

                for ( j;  j < rmax+y; j++) {

                        if (i != x || j != y) {
                                double r = sqrt(double((i-x)*(i-x) + (j-y)*(j-y)));
                                double val = (-level/10. * 1./r*value);
                                if (layer->getValue(i,j) > val ) {
                                        layer->setValue(i,j,val );
                                }
                        } else {
                                double val = (-level/10. *( 2.) * value);
                                if (layer->getValue(i,j) > val) {
                                        layer->setValue(i,j,val);
                                }
                        }
                }
        }
}

void GenerateSewerNetwork::MarkPathWithField(const std::vector<Pos> & path, RasterData * ConnectivityField, int ConnectivityWidth) {

    if (path.size() < 1) {
        Logger(vibens::Debug) << "MarkPathWithField: Path Size = 0" ;
        return;
    }
    int last = path.size() - 1;
    RasterData Buffer;
    Buffer.setSize(ConnectivityField->getWidth(), ConnectivityField->getHeight(), ConnectivityField->getCellSize());
    Buffer.clear();


    //Cost Function for Length
    double x = path[0].x - path[last].x;
    double y = path[0].y - path[last].y;

    //Calculate Optimal Length
    double r_opt = sqrt(x * x + y * y);


    for (int i = 0; i < path.size(); i++) {

        double val = 1 - ((double) i / (double) path.size());

        double r = last+1;

        val = val * r_opt / r;



       // GenerateSewerNetwork::addRadiusValueADD(
                //    path[last - i].x,  path[last - i].y, & Buffer, ConnectivityWidth, val);

        GenerateSewerNetwork::addRadiusValue(
                    path[last - i].x,  path[last - i].y, & Buffer, ConnectivityWidth, val);

    }
    x = path[last].x;
    y = path[last].y;
    for (int i = 0; i < ConnectivityField->getHeight(); i++) {
        for (int j = 0; j < ConnectivityField->getWidth(); j++) {

            double val = ConnectivityField->getValue(j, i);
            double val2 = Buffer.getValue(j, i);
            if (val > val2) {
                ConnectivityField->setValue(j, i, val2);
            }
        }
    }


}
int GenerateSewerNetwork::indexOfMinValue(const ublas::vector<double> &vec) {
    double val = vec[0];
    int index = 0;
    for (int i = 1; i < 9; i++) {
        if (vec[i] < val) {
            val = vec[i];
            index = i;
        }
    }

    //Check if alone
    int n = 0;
    for (int i = 0; i < 9; i++) {
        if (vec[i] == val) {
            n++;
        }
    }
    if (n > 1)
        index = -1;
    return index;
}


void GenerateSewerNetwork::run() {
    long width = this->Topology->getWidth();
    long height = this->Topology->getHeight();
    double cellSize = this->Topology->getCellSize();

    this->ConnectivityField->setSize(width, height, cellSize);
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++ ) {
            this->ConnectivityField->setValue(i,j, this->ConnectivityField_in->getValue(i,j));
        }
    }
    this->Path->setSize(width, height, cellSize);
    this->Path->clear();
    std::vector<Agent * > agents;

    std::vector<std::string> StartPosNames_tmp;
    StartPosNames_tmp = this->StartPoints->getPointsNames();
    std::vector<std::string> StartPosNames;
    foreach(std::string name, StartPosNames_tmp) {
        if (name.find(this->IdentifierStartPoins,0) == 0) {
            Attribute attr = this->StartPoints->getAttributes(name);
            if ((int)attr.getAttribute("New")  == 1)
                StartPosNames.push_back(name);
        }
    }

    //Create Agents
    foreach(std::string name, StartPosNames) {
        std::vector<Point> points = this->StartPoints->getPoints(name);
        foreach(Point p, points) {
            long x = (long) p.getX()/cellSize;
            long y = (long) p.getY()/cellSize;
            Agent * a = new Agent(Pos(x,y));
            a->Topology = this->Topology;
            a->MarkPath = this->Path;
            a->ConnectivityField = this->ConnectivityField;
            a->Goals = this->Goals;
            a->AttractionTopology = this->AttractionTopology;
            a->AttractionConnectivity = this->AttractionConnectivity;
            a->steps = this->steps;
            a->Hmin = this->Hmin;
            a->ForbiddenAreas = this->ForbiddenAreas;
            agents.push_back(a);

        }
    }
    long successfulAgents = 0;
    for (int i = 0; i < 1; i++) {
        for (int j = 0; j < agents.size(); j++) {
            Agent * a = agents[j];
            if (a->alive) {
                a->run();
            }
        }
        for (int j = 0; j < agents.size(); j++) {
            Agent * a = agents[j];
            if (a->successful) {
                successfulAgents++;
                GenerateSewerNetwork::MarkPathWithField(a->path, this->ConnectivityField, this->ConnectivityWidth);
            }
        }

    }
    Logger(vibens::Debug) << "Successful " << successfulAgents;
    this->sendDoubleValueToPlot(this->getInternalCounter(), (double) successfulAgents/agents.size());

    for (int j = 0; j < agents.size(); j++) {
        delete agents[j];
    }

    agents.clear();


}

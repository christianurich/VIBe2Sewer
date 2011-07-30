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
#include "networkanalysis.h"
#include "vectordatahelper.h"

VIBe_DECLARE_NODE_NAME(NetworkAnalysis, Sewer)
NetworkAnalysis::NetworkAnalysis()
{
    this->IdentifierConduit = "Conduit_";
    this->addParameter("Network", VIBe2::VECTORDATA_IN, &this->network, "Network");
    this->addParameter("Network_out", VIBe2::VECTORDATA_OUT, &this->network_out, "Network");
    this->addParameter("IdentifierConduit", VIBe2::STRING, & this->IdentifierConduit);

}

bool NetworkAnalysis::checkPoint(Point &p, double offset) {
    foreach (Point p1, this->PointList) {
        if (p.compare2d(p1, offset))
            return true;
    }
    return false;
}
int NetworkAnalysis::findNewID(Point &p, double offset) {
    int counter = 0;
    foreach (Point p1, this->PointList) {
        if (p.compare2d(p1, offset))
            return counter;
        counter++;
    }

    return -1;
}
std::vector<int> NetworkAnalysis::findConnectedEdges(int ID) {
    std::vector<int> ress;
    int counter = 0;
    foreach(Edge e, this->EdgeList) {
        if (e.getID1() == ID|| e.getID2() == ID) {
            ress.push_back(counter);
        }
        counter++;
    }

    return ress;
}


void NetworkAnalysis::run() {

    std::vector<Edge> sewerNetwork;
    PointList.clear();
    EdgeList.clear();
    std::vector<std::string> names = VectorDataHelper::findElementsWithIdentifier(this->IdentifierConduit, this->network->getEdgeNames());
    double offset = 10;
    //Create Point List
    foreach(std::string name , names) {
        std::vector<Edge> edges = this->network->getEdges(name);
        std::vector<Point> points = this->network->getPoints(name);
        foreach(Edge e, edges) {
            Point p1 = points[e.getID1()];
            Point p2 = points[e.getID2()];
            if (!this->checkPoint(p1, offset))
                this->PointList.push_back(p1);
            if (!this->checkPoint(p2, offset))
                this->PointList.push_back(p2);

            Edge e_new;
            e_new.id1 = this->findNewID(p1, offset);
            e_new.id2 = this->findNewID(p2, offset);

            foreach(Edge e, this->EdgeList) {
                if (e.getID1() == e_new.getID2() && e.getID2() == e_new.getID1()) {
                    Logger(vibens::Error) << "ARGHS";
                    //error = true;
                }
            }

            this->EdgeList.push_back(e_new);
        }


    }
    std::vector<int> StartNodes;
    //Find StartNodes
    foreach (Edge e, this->EdgeList) {
        if (this->findConnectedEdges(e.getID1()).size() == 1) {
            StartNodes.push_back(e.getID1());
        }
        if (this->findConnectedEdges(e.getID2()).size() == 1) {
            StartNodes.push_back(e.getID2());
        }
    }
    Logger(vibens::Debug) << "Number of StartNodes" << StartNodes.size();


    std::vector<int> StrahlerNumber;
    foreach(Edge e, this->EdgeList) {
        StrahlerNumber.push_back(0);
    }

    foreach(int StartID, StartNodes) {
        std::vector<int> ids = this->findConnectedEdges(StartID);
        foreach(int id, ids) {
            StrahlerNumber[id] = 1;
        }
    }

    foreach(int StartID, StartNodes) {
        std::vector<int> ids = this->findConnectedEdges(StartID);
        if (ids.size()!= 1) {
            Logger(vibens::Error) << "No Start Node";
            break;
        }

        int nextID = this->EdgeList[ids[0]].getID2();
        int prevStrahler = 1;

        std::vector<int> visitedNodes;
        do {
            visitedNodes.push_back(nextID);
            ids = this->findConnectedEdges(nextID);
            if (ids.size() == 1) {
                //Logger(vibens::Error) << "Unconnected Parts in Graph 1";
                break;
            }
            int nextid_tmp = -1;
            int outgoing_id;
            foreach(int id, ids) {
                if (this->EdgeList[id].getID2() != nextID) {
                    nextid_tmp = this->EdgeList[id].getID2();
                    outgoing_id = id;
                }

            }
            if (nextid_tmp == -1) {
                //Logger(vibens::Error) << "Unconnected Parts in Graph 2";
                break;
            }
            int maxStrahlerInNode = -1;
            int strahlerCounter = 0;
            foreach(int id, ids) {
                if (id != outgoing_id) {
                    if (maxStrahlerInNode < StrahlerNumber[id]) {
                        maxStrahlerInNode = StrahlerNumber[id];
                        strahlerCounter = 1;
                    } else if (maxStrahlerInNode == StrahlerNumber[id]) {
                        strahlerCounter++;
                    }

                }
            }
            if (strahlerCounter > 1) {
                if ( StrahlerNumber[outgoing_id] < maxStrahlerInNode+1 )
                    StrahlerNumber[outgoing_id] =  maxStrahlerInNode+1;

            } else {
                if (StrahlerNumber[outgoing_id] < prevStrahler) {
                    StrahlerNumber[outgoing_id] = prevStrahler;
                }
            }
            prevStrahler = StrahlerNumber[outgoing_id];
            nextID = nextid_tmp;


            foreach (int visited,visitedNodes) {
                if (nextID == visited) {
                    this->network_out->clear();
                    Logger(vibens::Error) << "Endless Loop";
                    return;
                }

            }

        } while (nextID != -1);

    }

    *(this->network_out) = *(this->network) ;

    //Add Strahler Number to Conduits

    //Find ConduitID


    for (int i = 0; i < this->EdgeList.size(); i++) {
        std::stringstream ss;
        Point p1(this->PointList[this->EdgeList[i].getID1()].x, this->PointList[this->EdgeList[i].getID1()].y ,1);
        Point p2(this->PointList[this->EdgeList[i].getID2()].x, this->PointList[this->EdgeList[i].getID2()].y, 1);
        std::string id = VectorDataHelper::findEdgeID(*(this->network), p1, p2, "Conduit_");

        Attribute attr (this->network->getAttributes(id) );

        attr.setAttribute("StrahlerNumber", StrahlerNumber[i]);
        this->network_out->setAttributes(id, attr);
    }


}

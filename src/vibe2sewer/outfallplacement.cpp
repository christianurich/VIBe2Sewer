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
#include "outfallplacement.h"
#include "vectordatahelper.h"
VIBe_DECLARE_NODE_NAME(OutfallPlacement, Sewer)
OutfallPlacement::OutfallPlacement()
{

    this->Identfier_Shaft = "Shaft_";
    this->Identfier_Conduit = "Conduit_";
    this->Identifier_Inlet = "Inlet_";
    this->Identfier_EndPoint = "EndPoint_";
    this->addParameter("Identifier_Shaft", VIBe2::STRING, &this->Identfier_Shaft);
    this->addParameter("Identifier_Conduit", VIBe2::STRING, &this->Identfier_Conduit);
    this->addParameter("Identifier_EndPoint", VIBe2::STRING, &this->Identfier_EndPoint);
    this->addParameter("Identifier_Inlet", VIBe2::STRING, &this->Identifier_Inlet);
    this->addParameter("Network", VIBe2::VECTORDATA_IN, & this->Network);
    this->addParameter("Network_out", VIBe2::VECTORDATA_OUT, & this->Network_out);
}

void OutfallPlacement::run() {
    *(this->Network_out) = *(this->Network);
    //Get Highest Strahler Number
    std::vector<std::string> names = VectorDataHelper::findElementsWithIdentifier(this->Identfier_Conduit, this->Network->getAttributeNames());

    int Strahler_Max = 0;
    foreach (std::string name, names) {
        Attribute attr = this->Network->getAttributes(name);
        if (Strahler_Max < attr.getAttribute("StrahlerNumber"))
            Strahler_Max = attr.getAttribute("StrahlerNumber");

    }
    int conduitcounter = 0;
    names = VectorDataHelper::findElementsWithIdentifier(this->Identfier_Conduit, this->Network->getEdgeNames());
    foreach (std::string name, names) {
        name.erase(0, this->Identfier_Conduit.size());
        int n = atoi(name.c_str());
        if (conduitcounter < n)
            conduitcounter = n;
    }
    conduitcounter++;



    int shaftcounter = 0;
    names = VectorDataHelper::findElementsWithIdentifier(this->Identfier_Shaft, this->Network->getPointsNames());
    foreach (std::string name, names) {
        name.erase(0, this->Identfier_Shaft.size());
        int n = atoi(name.c_str());
        if (shaftcounter < n)
            shaftcounter = n;
    }
    shaftcounter++;

    names = VectorDataHelper::findElementsWithIdentifier(this->Identifier_Inlet, this->Network->getPointsNames());
    std::vector<Point> New_Outfalls;
    std::vector<Point> WWTPs_Outfalls;
    foreach (std::string name, names) {
        Point p = this->Network->getPoints(name)[0];
        std::vector<std::string> connectedConduits = VectorDataHelper::findConnectedEges(*(this->Network), p, 10, LOWER, this->Identfier_Conduit);
        int prevStrahler = 1;
        std::vector<std::string> VisitedConduits;
        while (connectedConduits.size() > 0) {



            if (std::find(VisitedConduits.begin(), VisitedConduits.end(), connectedConduits[0]) != VisitedConduits.end()) {
                Logger(Error)<< "EndlessLoop";
                this->Network_out->clear();
                return;
            }
                VisitedConduits.push_back(connectedConduits[0]);
            Attribute attr = this->Network->getAttributes(connectedConduits[0]);

            int currentStrahler = attr.getAttribute("StrahlerNumber");

            Edge e = this->Network->getEdges(connectedConduits[0])[0];



            if (currentStrahler > prevStrahler && Strahler_Max == currentStrahler) {
                if (!VectorDataHelper::checkIfPointExists(New_Outfalls, p, 10) ) {

                    Logger(Debug) << "Place Outfall";
                    std::string nodename = VectorDataHelper::findPointID(*(this->Network), p, this->Identfier_Shaft);
                    Attribute node_attr = this->Network->getAttributes(nodename);

                    //Check if Outfall Exists
                    if (node_attr.getAttribute("Outfall") < 1) {
                        node_attr.setAttribute("Outfall", 1);
                        this->Network_out->setAttributes(nodename, node_attr);
                        New_Outfalls.push_back(p);
                    }
                }


            }



            p = this->Network->getPoints(connectedConduits[0])[1];
            prevStrahler = currentStrahler;
            connectedConduits = VectorDataHelper::findConnectedEges(*(this->Network), p, 10, LOWER, this->Identfier_Conduit);

            //Place WWTPs
            if (connectedConduits.size() == 0) {
                if (!VectorDataHelper::checkIfPointExists(WWTPs_Outfalls, p, 10) ) {
                    Logger(Debug) << "Place Outfall";
                    std::string nodename = VectorDataHelper::findPointID(*(this->Network), p, this->Identfier_Shaft);
                    Attribute node_attr = this->Network->getAttributes(nodename);
                    if (node_attr.getAttribute("WWTP") > 0) {
                         WWTPs_Outfalls.push_back(p);

                    }
                }
            }
        }
    }

    //Create Outfalls and Weirs
    int OutFallCounter = 0;
    foreach(Point p, New_Outfalls) {
        std::stringstream ss;
        ss << "Outfall_" << OutFallCounter;
        std::stringstream ss1;
        ss1 << "Weir_" << OutFallCounter;
        std::vector<Point> vp;
        Point OffestPoint(30,30,0);
        vp.push_back(p+OffestPoint);

        if (VectorDataHelper::findPointID(*(this->Network_out), p+OffestPoint, "Outfall_", 10).size() > 0)
            continue;

        this->Network_out->setPoints(ss.str(), vp);
        OutFallCounter++;
        Edge e;
        e.id1 = 0;
        e.id2 = 1;
        std::vector<Edge> ve;
        ve.push_back(e);



        std::vector<Point> vw;
        vw.push_back(p);
        vw.push_back(p+OffestPoint);

        this->Network_out->setEdges(ss1.str(),ve);
        this->Network_out->setPoints(ss1.str(), vw);
    }

    //Create WWTP
    OutFallCounter = 0;
    foreach(Point p, WWTPs_Outfalls) {
        std::stringstream ss;
        ss << "WWTP_" << OutFallCounter;
        std::stringstream ss1;
        ss1 << "WWTPConduit_" << OutFallCounter;
        std::vector<Point> vp;
        Point OffestPoint(-30,-30,0);
        vp.push_back(p+OffestPoint);

        if (VectorDataHelper::findPointID(*(this->Network_out), p+OffestPoint, "WWTP_", 10).size() > 0)
            continue;

        this->Network_out->setPoints(ss.str(), vp);
        OutFallCounter++;
        Edge e;
        e.id1 = 0;
        e.id2 = 1;
        std::vector<Edge> ve;
        ve.push_back(e);



        std::vector<Point> vw;
        vw.push_back(p);
        vw.push_back(p+OffestPoint);

        this->Network_out->setEdges(ss1.str(),ve);
        this->Network_out->setPoints(ss1.str(), vw);
    }
    //Create Outfall
    foreach(Point p, WWTPs_Outfalls) {
        std::stringstream ss;
        ss << "Outfall_" << OutFallCounter;
        std::stringstream ss1;
        ss1 << "Weir_" << OutFallCounter;
        std::vector<Point> vp;
        Point OffestPoint(30,30,0);
        vp.push_back(p+OffestPoint);

        if (VectorDataHelper::findPointID(*(this->Network_out), p+OffestPoint, "WWTP_", 10).size() > 0)
            continue;

        this->Network_out->setPoints(ss.str(), vp);
        OutFallCounter++;
        Edge e;
        e.id1 = 0;
        e.id2 = 1;
        std::vector<Edge> ve;
        ve.push_back(e);



        std::vector<Point> vw;
        vw.push_back(p);
        vw.push_back(p+OffestPoint);

        this->Network_out->setEdges(ss1.str(),ve);
        this->Network_out->setPoints(ss1.str(), vw);
    }

}

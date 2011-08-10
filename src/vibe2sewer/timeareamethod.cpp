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
#include "timeareamethod.h"
#include "vectordatahelper.h"
VIBe_DECLARE_NODE_NAME(TimeAreaMethod, Sewer)
TimeAreaMethod::TimeAreaMethod()
{
    this->IdentifierConduit = "Conduit_";
    this->IdentifierInlet = "Inlet_";
    this->IdentifierEndPoint = "EndPoint_";
    this->IdentifierShaft = "Shaft_";
    this->IdentifierCatchment = "Catchment_";
    //this->IdentifierOutlett = "Outlett_"
    this->v = 1;
    this->r15 = 150;
    this->addParameter("Network", VIBe2::VECTORDATA_IN, &this->Network);
    this->addParameter("Catchments", VIBe2::VECTORDATA_IN, &this->Catchments);
    this->addParameter("Network_out", VIBe2::VECTORDATA_OUT, &this->Network_out);
    this->addParameter("IdentifierInlet", VIBe2::STRING, &this->IdentifierInlet);
    this->addParameter("IdentifierConduit", VIBe2::STRING, &this->IdentifierConduit);
    this->addParameter("IdentifierEndPoint", VIBe2::STRING, &this->IdentifierEndPoint);
    this->addParameter("IdentifierShaft", VIBe2::STRING, &this->IdentifierShaft);
    this->addParameter("IdentifierCatchment", VIBe2::STRING, & this->IdentifierCatchment);
    //this->addParameter("IdentifierOutlett", VIBe2::STRING, &this->IdentifierOutlett);
    this->addParameter("v", VIBe2::DOUBLE, & this->v);
    this->addParameter("r15", VIBe2::DOUBLE, & this->r15);
}

double TimeAreaMethod::caluclateAPhi(const Attribute & attr, double r15)  const {

    double n = 1.;
    double T = 10.;
    double phi10 = (38./(T+9.)*(pow(n,(-1/4))-0,369));
    //double r10 = r15 * phi10;

    T = attr.getAttribute("Time") / 60.;
    if (T < 10) {
        T = 10;
    }
    double phiT = (38./(T+9.)*(pow(n,(-1/4))-0,369));

    return phiT/phi10;


}

bool TimeAreaMethod::checkPoint(Point &p) {
    foreach (Point p1, this->PointList) {
        if (p.compare2d(p1))
            return true;
    }
    return false;

}
int TimeAreaMethod::findNewID(Point &p) {
    int counter = 0;
    foreach (Point p1, this->PointList) {
        if (p.compare2d(p1))
            return counter;
        counter++;
    }

    return -1;
}
int TimeAreaMethod::findDownStreamNode(int ID) {
    std::vector<int> ids = this->findConnectedEdges(ID);

    foreach(int id, ids) {
        Edge e = this->EdgeList[id];
        if (e.getID2() != ID)
            return e.getID2();
    }

    return -1;

}

std::vector<int> TimeAreaMethod::findConnectedEdges(int ID) {
    std::vector<int> ress;
    int counter = 0;
    foreach(Edge e, this->EdgeList) {
        if (e.getID1() == ID || e.getID2() == ID) {
            ress.push_back(counter);
        }
        counter++;
    }

    return ress;
}

void TimeAreaMethod::run() {
    //Generate List Of Start Points

    //Calculate Waste Water
    std::vector<std::string> InletNames;
    InletNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierInlet,this->Network->getPointsNames());
    double Population_sum = 0;

    //double WasterWaterPerPerson = 0.0052;
    double InfiltrationWater =  0.003;

    *(this->Network_out) = *(this->Network);

    //Calculate Water Water per Node
    foreach(std::string name, InletNames) {
        Attribute inlet_attr;
        inlet_attr = this->Network->getAttributes(name);
        std::string id;
        id = QString::number( (int) inlet_attr.getAttribute(this->IdentifierCatchment+"ID") ).toStdString();

        Attribute catchment_attr;
        catchment_attr = this->Catchments->getAttributes(this->IdentifierCatchment+id);
        double pop =  catchment_attr.getAttribute("Population");
        double area = catchment_attr.getAttribute("Area");
        if(catchment_attr.getAttribute("Impervious") == 0) {
            catchment_attr.setAttribute("Impervious", 0.2);
        }
        double imp = catchment_attr.getAttribute("Impervious") - catchment_attr.getAttribute("RoofAreaInfitrated")/(200*200);
        double ww = catchment_attr.getAttribute("WW");

        inlet_attr.setAttribute("InfiltrationWater", pop * InfiltrationWater);
        inlet_attr.setAttribute("WasterWater", ww);
        inlet_attr.setAttribute("Area",area*imp);
        inlet_attr.setAttribute("QrKrit", 15.*area/10000*imp);

        this->Network_out->setAttributes(name, inlet_attr);
        this->Network_out->setAttributes(this->IdentifierCatchment+id, catchment_attr);
        Population_sum += catchment_attr.getAttribute("Population");

    }
    //Create Point List

    foreach(std::string name , VectorDataHelper::findElementsWithIdentifier(this->IdentifierConduit,this->Network->getEdgeNames()))
    {
        std::vector<Edge> edges = this->Network->getEdges(name);
        std::vector<Point> points = this->Network->getPoints(name);
        foreach(Edge e, edges) {
            Point p1 = points[e.getID1()];
            Point p2 = points[e.getID2()];
            if (!this->checkPoint(p1))
                this->PointList.push_back(p1);
            if (!this->checkPoint(p2))
                this->PointList.push_back(p2);

            Edge e_new;
            e_new.id1 = this->findNewID(p1);
            e_new.id2 = this->findNewID(p2);

            //Check
            bool error = false;
            foreach(Edge e, this->EdgeList) {
                if (e.getID1() == e_new.getID2() && e.getID2() == e_new.getID1()) {
                    Logger(vibens::Error) << "ARGHS";
                    error = true;
                }
            }
            //if (!error)
            this->EdgeList.push_back(e_new);
        }
    }

    std::vector<double> WasteWaterPerShaft;
    std::vector<double> AreaPerShaft;
    std::vector<double> InfiltrartionWaterPerShaft;
    std::vector<double> QrKritPerShaft;
    std::vector<double> QrKritPerShaft_total;
    std::vector<double> Area_total;
    std::vector<double> Lengths;
    std::vector<std::vector<string> > ConnectedInletNodes;

    for (int i = 0; i < this->PointList.size(); i++) {
        WasteWaterPerShaft.push_back(0);
        InfiltrartionWaterPerShaft.push_back(0);
        AreaPerShaft.push_back(0);
        Lengths.push_back(0);
        QrKritPerShaft.push_back(0);
        QrKritPerShaft_total.push_back(0);
        Area_total.push_back(0);
        std::vector<std::string> vec;
        ConnectedInletNodes.push_back(vec);

    }
    std::vector<std::string> endPointNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierEndPoint,this->Network->getPointsNames());
    foreach(std::string name, endPointNames) {
        std::vector<Point> points = this->Network->getPoints(name);
        foreach(Point p, points) {
            this->EndPointList.push_back(p);
        }
    }

    foreach(std::string name, InletNames) {
        Attribute inlet_attr;
        inlet_attr = this->Network_out->getAttributes(name);

        double wastewater = inlet_attr.getAttribute("WasterWater");
        double infiltreationwater = inlet_attr.getAttribute("InfiltrationWater");
        double area = inlet_attr.getAttribute("Area");
        double QrKrit = inlet_attr.getAttribute("QrKrit");
        std::vector<Point> vstartp = this->Network->getPoints(name);

        if (vstartp.size() != 1)
            return;

        Point startp = vstartp[0];

        int id = this->findNewID(startp);

        bool ReachedEndPoint = false;
        int idPrev = -1;
        double StrangL = 0;
        double QKrit = 0;
        double DeltaA = 0;
        do {
            //Check if Endpoint is reached
            std::vector<std::string> nodes = ConnectedInletNodes[id];
            nodes.push_back( name );
            ConnectedInletNodes[id] = nodes;
            WasteWaterPerShaft[id]=WasteWaterPerShaft[id]+ wastewater;
            InfiltrartionWaterPerShaft[id] = InfiltrartionWaterPerShaft[id] + infiltreationwater;
            AreaPerShaft[id] =  AreaPerShaft[id] + area;
            QrKritPerShaft[id] = QrKritPerShaft[id] + QrKrit;

            if (idPrev != -1) {
                Point dp = this->PointList[idPrev] - this->PointList[id];
                StrangL += sqrt(dp.x*dp.x + dp.y*dp.y + dp.z*dp.z);
            }

            //Search for Outfall
            std::string name = VectorDataHelper::findPointID(*(this->Network), this->PointList[id], this->IdentifierShaft);
            Attribute attr = this->Network->getAttributes(name);
            if ((int) attr.getAttribute("Outfall") == 1) {
                StrangL = 0;
                QKrit = QrKritPerShaft[id];
                DeltaA = AreaPerShaft[id];
            }

            if (Lengths[id] < StrangL)
                Lengths[id] = StrangL;


            QrKritPerShaft_total[id] = QKrit;
            Area_total[id] = AreaPerShaft[id] - DeltaA;

            idPrev = id;
            id = this->findDownStreamNode(id);

            if (id == -1) {
                break;
            }




            foreach(Point p, this->EndPointList) {
                if (id == this->findNewID(p) ){
                    ReachedEndPoint = true;
                    WasteWaterPerShaft[id]= WasteWaterPerShaft[id]+wastewater;
                    InfiltrartionWaterPerShaft[id] = InfiltrartionWaterPerShaft[id] + infiltreationwater;
                    AreaPerShaft[id] =  AreaPerShaft[id] + area;

                    if (idPrev != -1) {
                        Point dp = this->PointList[idPrev] - this->PointList[id];
                        StrangL += sqrt(dp.x*dp.x + dp.y*dp.y + dp.z*dp.z);
                    }


                    if (Lengths[id] < StrangL)
                        Lengths[id] = StrangL;

                    break;
                }
            }



        }while (!ReachedEndPoint && id >= 0);
        if (id == -1)
            continue;
    }

    //Write Data to shaft
    for (int i = 0; i < this->PointList.size(); i++) {
        Point p = this->PointList[i];
        std::string id;
        foreach(std::string name,VectorDataHelper::findElementsWithIdentifier(this->IdentifierShaft,this->Network->getPointsNames())) {
            Point p_tmp = this->Network->getPoints(name)[0];
            if (p.compare2d(p_tmp)) {
                id = name;
                break;
            }
        }

        Attribute attr;

        attr = this->Network->getAttributes(id);


        attr.setAttribute("WasterWater", WasteWaterPerShaft[i]);
        attr.setAttribute("InfitrationWater", InfiltrartionWaterPerShaft[i]);
        attr.setAttribute("Area", AreaPerShaft[i]);
        attr.setAttribute("Time", Lengths[i]/this->v + sqrt(AreaPerShaft[i]));
        attr.setAttribute("APhi", this->caluclateAPhi(attr, this->r15));
        attr.setAttribute("QrKrit", QrKritPerShaft[i]);
        attr.setAttribute("QrKrit_total", QrKritPerShaft_total[i]* (45./(attr.getAttribute("Time")/60 + 30)));
        attr.setAttribute("Area_total", Area_total[i]);

        //Write Connected Nodes
        std::stringstream connectedNodeList;
        for (int j = 0; j < ConnectedInletNodes[i].size(); j++) {
            if (j != 0)
                connectedNodeList << ",";
            connectedNodeList << ConnectedInletNodes[i][j];

        }
        attr.setAttribute("ConnectedInlets", connectedNodeList.str());

        attr.setAttribute("StorageV",0);
        attr.setAttribute("Storage",0);
        if (attr.getAttribute("WWTP") == 1) {
            attr.setAttribute("Storage",1);
            attr.setAttribute("StorageV",attr.getAttribute("QrKrit_total"));
        }

        this->Network_out->setAttributes(id, attr);

    }

    //Dimensioning
    foreach(Edge e, EdgeList) {
        Point p = this->PointList[e.getID1()];
        std::string id;
        foreach(std::string name,VectorDataHelper::findElementsWithIdentifier(this->IdentifierShaft,this->Network->getPointsNames())) {
            Point p_tmp = this->Network->getPoints(name)[0];
            if (p.compare2d(p_tmp)) {
                id = name;
                break;
            }
        }

        Attribute attr;
        attr = this->Network_out->getAttributes(id);


        std::string id_cond = VectorDataHelper::findEdgeID(*(this->Network_out), p, this->PointList[e.getID2()], this->IdentifierConduit);

        //Get Existing Attribute
        //Only change Diamater if Dimaeter = 0 -> newly placed Pipe, or if redesign is set true
        Attribute attr_cond = this->Network_out->getAttributes(id_cond);
        if (attr_cond.getAttribute("Diameter") == 0  || attr_cond.getAttribute("Redesign") == 1)
        {
            double QWasteWater = attr.getAttribute("WasterWater") +  attr.getAttribute("InfitrationWater");
            double QRainWater =  attr.getAttribute("Area_total")*attr.getAttribute("APhi")*this->r15/10000. +  attr.getAttribute("QrKrit_total");

            double QBem = (QRainWater + QWasteWater)/1000.; //m³

            attr_cond.setAttribute("Diameter", this->choosDiameter(sqrt((QBem)/3.14*4))); //in mm

            this->Network_out->setAttributes(id_cond, attr_cond);
        }

    }
    //Create Storage Building at the End
    std::vector<std::string> wwtp_con_names = VectorDataHelper::findElementsWithIdentifier("WWTPConduit", this->Network_out->getEdgeNames());
    foreach(std::string name, wwtp_con_names) {
        std::vector<Point> points = this->Network_out->getPoints(name);
        Point p = points[0];
        Attribute attr = VectorDataHelper::findAttributeFromPoints(*this->Network_out, p, this->IdentifierShaft, 10);
        double QBem = 2* (attr.getAttribute("WasterWater") +  attr.getAttribute("InfitrationWater"))/1000.;
        Attribute attr_cond = this->Network_out->getAttributes(name);
        attr_cond.setAttribute("Diameter", this->choosDiameter(sqrt((QBem)/3.14*4)));
        this->Network_out->setAttributes(name, attr_cond);
    }
    Logger(vibens::Standard) << "Sum over Population " << Population_sum;
}

double TimeAreaMethod::choosDiameter(double diameter) {
    QVector<double> vd;
    //vd.append(150);
    //vd.append(200);
    //vd.append(250);
    vd.append(300);
    vd.append(350);
    vd.append(400);
    vd.append(450);
    vd.append(500);
    vd.append(600);
    vd.append(700);
    vd.append(800);
    vd.append(900);
    vd.append(1000);
    vd.append(1100);
    vd.append(1200);
    vd.append(1300);
    vd.append(1400);
    vd.append(1500);
    vd.append(1600);
    vd.append(1800);
    vd.append(2000);
    vd.append(2200);
    vd.append(2400);
    vd.append(2600);
    vd.append(2800);
    vd.append(3000);
    vd.append(3200);
    vd.append(3400);
    vd.append(3600);
    vd.append(3800);
    vd.append(4000);

    double d = 0;
    for (int i = 0; i < vd.size(); i++) {
        if (d == 0 && diameter*1000 <= vd.at(i) ) {
            d =  vd.at(i);
        }
    }
    if (d == 0.) {
        d = 4000;
    }
    return d;

};

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
#include "vibeswmm.h"
#include <fstream>
#include "vectordatahelper.h"
#include <QDir>
#include <QUuid>
#include <QProcess>
#include <QTextStream>
#include <QSettings>

VIBe_DECLARE_NODE_NAME(VibeSWMM, Sewer)
VibeSWMM::VibeSWMM()
{
    this->IdentifierConduit = "Conduit_";
    this->IdentifierInlet = "Inlet_";
    this->IdentifierEndPoint = "EndPoint_";
    this->IdentifierShaft = "Shaft_";
    this->IdentifierCatchment = "Catchment_";
    this->IdentifierOutfall = "Outfall_";
    this->IdentifierWier = "Weir_";
    this->FileName = "swmmfile";
    this->climateChangeFactor = 1;
    this->RainFile = "";
    this->Vr = 0;
    this->Vp = 0;
    this->Vwwtp = 0;
    years = 0;
    this->addParameter("Network", VIBe2::VECTORDATA_IN, &this->Network);
    this->addParameter("Catchments", VIBe2::VECTORDATA_IN, &this->Catchments);
    this->addParameter("Network_out", VIBe2::VECTORDATA_OUT, &this->Network_out);
    this->addParameter("FileName", VIBe2::STRING, &this->FileName);
    this->addParameter("RainFile", VIBe2::FILENAME, &this->RainFile);
    this->addParameter("IdentifierInlet", VIBe2::STRING, &this->IdentifierInlet);
    this->addParameter("IdentifierConduit", VIBe2::STRING, &this->IdentifierConduit);
    this->addParameter("IdentifierEndPoint", VIBe2::STRING, &this->IdentifierEndPoint);
    this->addParameter("IdentifierShaft", VIBe2::STRING, &this->IdentifierShaft);
    this->addParameter("IdentifierCatchment", VIBe2::STRING, & this->IdentifierCatchment);
    this->addParameter("IdentifierOutfall", VIBe2::STRING,  & this->IdentifierOutfall);
    this->addParameter("IdentifierWeir", VIBe2::STRING, & this->IdentifierWier);
    this->addParameter("ClimateChangeFactor", VIBe2::DOUBLE, & this->climateChangeFactor);
    this->addParameter("Vr", VIBe2::DOUBLEDATA_OUT, & this->Vr);
    this->addParameter("Vp", VIBe2::DOUBLEDATA_OUT, & this->Vp);
    this->addParameter("Vwwtp", VIBe2::DOUBLEDATA_OUT, & this->Vwwtp);
    counterRain =0;

}
void VibeSWMM::writeRainFile() {
    QFile r_in;
    r_in.setFileName(QString::fromStdString(this->RainFile));
    r_in.open(QIODevice::ReadOnly);
    QTextStream in(&r_in);

    QString line;
    QString fileName = this->SWMMPath.absolutePath()+ "/"+ "rain.dat";
    std::fstream out;
    out.open(fileName.toAscii(),ios::out);

    do {
        line = in.readLine();
        QStringList splitline = line.split(QRegExp("\\s+"));
        if (splitline.size() > 2) {
            for(int i = 0; i < splitline.size() - 1;i++)
                out << QString(splitline[i]).toStdString() << " ";
            double r = QString(splitline[splitline.size()-1]).toDouble();
            //out << r*(1+0.025*counterRain) << "\n";
            out << r*(this->climateChangeFactor) << "\n";
        }

    } while (!line.isNull());

    r_in.close();
    out.close();
    counterRain++;
}

void VibeSWMM::run() {

    *(this->Network_out) = *(this->Network);
    QDir tmpPath = QDir::tempPath();
    QString UUIDPath = QUuid::createUuid().toString().remove("{").remove("}");

    tmpPath.mkdir(UUIDPath);
    this->SWMMPath.setPath(tmpPath.absolutePath() + "/" + UUIDPath);
    //CreatePointList and ConduitList
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
    this->writeSWMMFile();
    this->writeRainFile();
    this->RunSWMM();
    this->readInReportFile();
}
void VibeSWMM::readInReportFile() {

    this->reportFile.setFileName(this->SWMMPath.absolutePath() + "/" + "swmm.rep");
    this->reportFile.open(QIODevice::ReadOnly);
    QTextStream in(&this->reportFile);

    bool erroraccrued = false;
    QStringList fileContent;

    QString line;

    do {
        line = in.readLine();
        fileContent << line;
    } while (!line.isNull());

    this->reportFile.close();

    //Search for Errors

    foreach (QString l, fileContent) {
        if (l.contains("WARNING"))
            Logger(Warning) << l.toStdString();
        if (l.contains("ERROR")) {
            Logger(Error) << l.toStdString();
            erroraccrued = true;
        }

    }
    if (erroraccrued) {
        Logger(Error) << "Error in SWMM";
        return;
    }

    //Start Reading Input
    //Flooded Nodes
    bool FloodSection = false;
    bool SectionSurfaceRunoff = false;
    bool SectionOutfall = false;
    double SurfaceRunOff = 0;
    double Vp = 0;
    double Vr = 0;
    double Vwwtp = 0;

    int counter = 0;
    for (int i = 0; i < fileContent.size(); i++) {
        line = fileContent[i];
        if (line.contains("Runoff Quantity Continuity") ) {
            SectionSurfaceRunoff = true;
            continue;
        }
        if (line.contains("Outfall Loading Summary") ) {
            SectionOutfall = true;
            continue;
        }
        if (line.contains("Node Flooding Summary") ) {
            FloodSection = true;
            continue;
        }
        if (SectionSurfaceRunoff) {
            if (line.contains("Surface Runoff")) {
                //Start extract
                QStringList data =line.split(QRegExp("\\s+"));
                SurfaceRunOff = QString(data[data.size()-2]).toDouble()*10;
            }
        }
        if (SectionOutfall) {
            if (line.contains("OUTFALL")) {
                //Start extract
                QStringList data =line.split(QRegExp("\\s+"));
                for (int j = 0; j < data.size(); j++) {
                    if (data[j].size() == 0) {
                        data.removeAt(j);
                    }
                }
                //Extract Node id
                if (data.size() != 11) {
                    Logger(Error) << "Error in Extraction Outfall Nodes";

                } else {
                    QString id_asstring = data[0];
                    id_asstring.remove("OUTFALL");
                    int id = id_asstring.toInt();
                    Point p = this->PointList[id];
                    std::string nodename = VectorDataHelper::findPointID( *(this->Network_out), p, this->IdentifierOutfall);
                    Attribute attr = this->Network_out->getAttributes(nodename);

                    attr.setAttribute("OutfallVolume",  QString(data[4]).toDouble());

                    this->Network_out->setAttributes(nodename, attr);

                }


            }
            if (line.contains("WWTP")) {
                //Start extract
                QStringList data =line.split(QRegExp("\\s+"));
                for (int j = 0; j < data.size(); j++) {
                    if (data[j].size() == 0) {
                        data.removeAt(j);
                    }
                }
                //Extract Node id
                if (data.size() != 11) {
                    Logger(Error) << "Error in Extraction Outfall Nodes";

                } else {
                    QString id_asstring = data[0];
                    id_asstring.remove("WWTP");
                    int id = id_asstring.toInt();
                    Point p = this->PointList[id];
                    std::string nodename = VectorDataHelper::findPointID( *(this->Network_out), p,"WWTP_");
                    Attribute attr = this->Network_out->getAttributes(nodename);

                    attr.setAttribute("OutfallVolume",  QString(data[4]).toDouble());
                    Vwwtp += QString(data[4]).toDouble();
                    this->Network_out->setAttributes(nodename, attr);
                }


            }

        }
        if (FloodSection) {
            if (line.contains("NODE")) {
                //Start extract
                QStringList data =line.split(QRegExp("\\s+"));
                for (int j = 0; j < data.size(); j++) {
                    if (data[j].size() == 0) {
                        data.removeAt(j);
                    }
                }
                //Extract Node id
                if (data.size() != 7) {
                    Logger(Error) << "Error in Extraction Flooded Nodes";

                } else {
                    QString id_asstring = data[0];
                    id_asstring.remove("NODE");
                    int id = id_asstring.toInt();
                    Point p = this->PointList[id];
                    std::string nodename = VectorDataHelper::findPointID( *(this->Network_out), p, this->IdentifierShaft);
                    Attribute attr = this->Network_out->getAttributes(nodename);

                    attr.setAttribute("FloodVolume",  QString(data[5]).toDouble());
                    this->Network_out->setAttributes(nodename, attr);
                    Vp += QString(data[5]).toDouble();

                }


            }


        }
        if (counter > 0 && line.contains("************")) {
            FloodSection = false;
            SectionSurfaceRunoff = false;
            SectionOutfall = false;
            counter = -1;
        }
        counter ++;
    }
    Logger (Standard)  << "Vp " << Vp;
    Logger (Standard)  << "Vr " << SurfaceRunOff;
    Logger (Standard)  << "Vwwtp " << Vwwtp;
    this->Vp = Vp;
    this->Vwwtp = Vwwtp;
    this->Vr = SurfaceRunOff;
    this->sendDoubleValueToPlot(years, 1.-Vp/SurfaceRunOff);

    years++;





}

void VibeSWMM::RunSWMM() {
    this->Vp = 0;
    this->Vwwtp = 0;
    this->Vr = 0;
    //Find Temp Directory
    QDir tmpPath = QDir::tempPath();
    tmpPath.mkdir("vibeswmm");

    //Path to SWMM
    QSettings settings("IUT", "VIBe2");
    QString swmmPath = settings.value("SWMM").toString().replace("\\","/");
    if (swmmPath.lastIndexOf("/") != swmmPath.size()) {
        swmmPath.append("/");
    }

    //Copy SWMM to tmp Path
    QString newFileName = this->SWMMPath.absolutePath() + "/"+ "swmm5.dll";
    QFile (swmmPath+"swmm5.dll").copy(newFileName);

    newFileName = this->SWMMPath.absolutePath()+  + "/"+ "swmm5.exe";
    QFile (swmmPath+"swmm5.exe").copy(newFileName);

    //newFileName  = this->SWMMPath.absolutePath()+ "/" + "rain.dat";
    //QFile (QString::fromStdString(this->RainFile)).copy(newFileName);


    QProcess process;
    QStringList argument;
    argument << this->SWMMPath.absolutePath() + "/" + "swmm5.exe" << this->SWMMPath.absolutePath() + "/"+ "swmm.inp" << this->SWMMPath.absolutePath() + "/" + "swmm.rep";
#ifdef _WIN32
    process.start("swmm5",argument);
#else
    process.start("wine",argument);
#endif
    process.waitForFinished(300000);




}

int VibeSWMM::findNewID(const Point &p) {
    int counter = 0;
    foreach (Point p1, this->PointList) {
        if (p.compare2d(p1))
            return counter;
        counter++;
    }

    return -1;
}
void VibeSWMM::writeJunctions(std::fstream &inp)
{
    std::vector<std::string> JunctionNames;
    JunctionNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierShaft,this->Network->getPointsNames());
    QStringList l;
    std::vector<std::string> names = VectorDataHelper::findElementsWithIdentifier("Shaft_", this->Network->getPointsNames());
    foreach (std::string name, names) {
        QString s = QString::fromStdString(name);
        if (l.indexOf(s) >= 0) {
            Logger(Error) << "Duplicated Entry";
        }
        l.append(s);
    }
    std::vector<int> checkForduplicatedNodes;
    inp<<"[JUNCTIONS]\n";
    inp<<";;Name\t\tElev\tYMAX\tY0\tYsur\tApond\n";
    inp<<";;============================================================================\n";
    foreach(std::string name, JunctionNames) {
        Point p = this->Network->getPoints(name)[0];
        Attribute attr = this->Network->getAttributes(name);
        if (attr.getAttribute("Storage") == 1)
            continue;
        //SewerTree::Node *node = this->sewerTree.getNodes().at(i);

        /* double z = node->z;
             if (z < 1) {
                 std::cout << "Height Error" << endl;
             }*/
        double depht = p.z;
        int id = this->findNewID(p);

        if (std::find(checkForduplicatedNodes.begin(), checkForduplicatedNodes.end(), id) != checkForduplicatedNodes.end())
            continue;

        checkForduplicatedNodes.push_back(id);
        inp << "NODE";
        inp << id;
        inp << "\t";
        inp << "\t";
        //Get Val
        inp << attr.getAttribute("Z")-(attr.getAttribute("D")-2);

        inp << "\t";
        inp <<  attr.getAttribute("D")+1;
        inp << "\t";
        inp << "0";
        inp << "\t";
        inp << "0";
        inp << "\t";
        inp << "100000";
        inp << "\n";
    }
}


void VibeSWMM::writeSubcatchments(std::fstream &inp)
{
    int subcatchCount=0;


    //SUBCATCHMENTS
    //-------------------------//
    inp<<"[SUBCATCHMENTS]\n";
    inp<<";;Name\traingage\tOutlet\tArea\tImperv\tWidth\tSlope\n";
    inp<<";;============================================================================\n";
    //for (int i=0;i<subcatchCount;i++)
    //{
    //	inp<<"  sub"<<i<<"\tRG01"<<"\t\tnode"<<i<<"\t20\t80.0\t100.0\t1.0\t1\n";
    //}

    std::vector<std::string> InletNames;
    InletNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierInlet,this->Network->getPointsNames());

    foreach(std::string name, InletNames) {
        Attribute inlet_attr;
        inlet_attr = this->Network->getAttributes(name);
        std::vector<Point> points = this->Network->getPoints(name);

        std::string id_catchment;
        id_catchment = QString::number( (int) inlet_attr.getAttribute(this->IdentifierCatchment+"ID") ).toStdString();


        Attribute catchment_attr = this->Catchments->getAttributes(this->IdentifierCatchment+id_catchment);

        int id = this->findNewID(points[0]);
        double area = catchment_attr.getAttribute("Area")/10000.;// node->area/10000.;
        double with = sqrt(area*10000.);
        double gradient = abs(catchment_attr.getAttribute("Gradient"));
        double imp = catchment_attr.getAttribute("Impervious");
        if (imp < 0.2)
            imp = 0.2;
        if (gradient > 0.01)
            gradient = 0.01;

        if ( area > 0 ) {
            inp<<"sub"<<id_catchment<<"\tRG01"<<"\t\tnode"<<id<<"\t" << area << "\t" <<imp*100 << "\t"<< with << "\t"<<gradient*100<<"\t1\n";
            //inp<<"sub"<<subcatchCount++<<"\tRG01"<<"\t\tnode"<<id<<"\t" << area << "\t" <<catchment_attr.getAttribute("Impervious")*100 << "\t"<< with << "\t"<<gradient*100<<"\t1\n";
        }

    }
    inp<<"[POLYGONS]\n";
    inp<<";;Subcatchment\tX-Coord\tY-Coord\n";
    int counter = 0;
    foreach(std::string name, InletNames) {
        Attribute inlet_attr;
        inlet_attr = this->Network->getAttributes(name);
        std::string id;
        id = QString::number( (int) inlet_attr.getAttribute(this->IdentifierCatchment+"ID") ).toStdString();
        std::vector<Point> points = this->Catchments->getPoints(this->IdentifierCatchment+id);
        foreach(Point  p, points)

            inp << "sub" << id <<"\t" << p.x << "\t" << p.y<< "\n";

        counter++;

    }

    inp<<"\n";
    //-------------------------//

    //SUBAREAS
    //-------------------------//
    inp<<"[SUBAREAS]\n";
    inp<<";;Subcatch\tN_IMP\tN_PERV\tS_IMP\tS_PERV\t%ZER\tRouteTo\n";
    inp<<";;============================================================================\n";
    foreach(std::string name, InletNames) {
        Attribute inlet_attr;
        inlet_attr = this->Network->getAttributes(name);
        std::string id;
        id = QString::number( (int) inlet_attr.getAttribute(this->IdentifierCatchment+"ID") ).toStdString();
        inp<<"  sub"<<id<<"\t\t0.015\t0.2\t1.8\t5\t0\tOUTLET\n";
    }
    inp<<"\n";
    //-------------------------//


    //INFILTRATION
    //-------------------------//
    inp<<"[INFILTRATION]\n";
    inp<<";;Subcatch\tMaxRate\tMinRate\tDecay\tDryTime\tMaxInf\n";
    inp<<";;============================================================================\n";
    foreach(std::string name, InletNames) {
        Attribute inlet_attr;
        inlet_attr = this->Network->getAttributes(name);
        std::string id;
        id = QString::number( (int) inlet_attr.getAttribute(this->IdentifierCatchment+"ID") ).toStdString();

        inp<<"  sub"<<id<<"\t60\t6.12\t3\t6\t0\n";
    }
    inp<<"\n";

}

void VibeSWMM::writeDWF(std::fstream &inp) {
    std::vector<std::string> InletNames;
    InletNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierInlet,this->Network->getPointsNames());
    //DWF Dry Weather Flow
    //-------------------------//

    inp<<"[DWF]\n";
    inp<<";;Node\tItem\tValue\n";
    inp<<";;============================================================================\n";

    foreach(std::string name, InletNames) {
        Attribute inlet_attr;
        inlet_attr = this->Network->getAttributes(name);
        std::vector<Point> points = this->Network->getPoints(name);

        int id = this->findNewID(points[0]);

        double Q = inlet_attr.getAttribute("WasteWater");
        if (Q > 0.000001) {
            inp<<"NODE"<<id<<"\tFLOW\t"<<Q<<"\n";
        }
    }
    inp<<"\n";
    //-------------------------//

    //POLLUTANTS
    //-------------------------//
    inp<<"[POLLUTANTS]\n";
    inp<<";;Name\tUnits\tCppt\tCgw\tCii\tKd\tSnow\tCoPollut\tCoFract\n";
    inp<<";;============================================================================\n";
    inp<<"ssolid\tMG/L\t0\t0\t0\t0\tNO\n";
    inp<<"vssolid\tMG/L\t0\t0\t0\t0\tNO\n";
    inp<<"cod\tMG/L\t0\t0\t0\t0\tNO\n";
    inp<<"cods\tMG/L\t0\t0\t0\t0\tNO\n";
    inp<<"nh4\tMG/L\t0\t0\t0\t0\tNO\n";
    inp<<"no3\tMG/L\t0\t0\t0\t0\tNO\n";
}
void VibeSWMM::writeStorage(std::fstream &inp) {
    //-------------------------//

    //STROGAE
    //-------------------------//
    inp<<"\n";
    inp<<"[STORAGE]\n";
    inp<<";;               Invert   Max.     Init.    Shape      Shape                      Ponded   Evap.\n"  ;
    inp<<";;Name           Elev.    Depth    Depth    Curve      Params                     Area     Frac. \n"  ;
    inp<<";;-------------- -------- -------- -------- ---------- -------- -------- -------- -------- --------\n";
    //\nODE85           93.7286  6.35708  0        FUNCTIONAL 1000     0        22222    1000     0
    std::vector<std::string> JunctionNames;
    JunctionNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierShaft,this->Network->getPointsNames());
    foreach(std::string name, JunctionNames) {
        Point p = this->Network_out->getPoints(name)[0];
        Attribute attr = this->Network_out->getAttributes(name);
        if (attr.getAttribute("Storage") != 1)
            continue;
        std::vector<std::string> n_es = VectorDataHelper::findConnectedEges(*this->Network, p);
        double diameter = 0;
        foreach (std::string n_e, n_es) {
            Attribute attr_e = this->Network->getAttributes(n_e);
            if (attr_e.getAttribute("Diameter") > diameter)
                diameter = attr_e.getAttribute("Diameter");
        }

        double depht = p.z;
        int id = this->findNewID(p);

        inp << "NODE";
        inp << id;
        inp << "\t";
        inp << "\t";
        //Get Val
        inp << attr.getAttribute("Z")-(attr.getAttribute("D")-2);

        inp << "\t";
        inp <<  attr.getAttribute("D")+5;
        inp << "\t";
        inp << "0";
        inp << "\t";
        inp << "FUNCTIONAL";
        inp << "\t";
        inp << "1000";
        inp << "\t";
        inp << "0";
        inp << "\t";

        inp << attr.getAttribute("StorageV")/diameter*1000*0.6;
        inp << "\t";
        inp << "1000";
        inp << "0";
        inp << "\n";
    }
}

void VibeSWMM::writeOutfalls(std::fstream &inp) {
    //OUTFALLS
    //-------------------------//
    inp<<"[OUTFALLS]\n";
    inp<<";;Name	Elev	Stage	Gate\n";
    inp<<";;============================================================================\n";

    std::vector<std::string> OutfallNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierOutfall,this->Network->getPointsNames());
    for ( int i = 0; i < OutfallNames.size(); i++ ) {
        Point p = this->Network->getPoints(OutfallNames[i])[0];

        double z = p.getZ()-4.;
        inp<<"OUTFALL"<<i<<"\t"<< z <<"\tFREE\tNO\n";
    }
    std::vector<std::string> WWTPs = VectorDataHelper::findElementsWithIdentifier("WWTP_",this->Network->getPointsNames());
    for ( int i = 0; i < WWTPs.size(); i++ ) {
        Point p = this->Network->getPoints(WWTPs[i])[0];

        double z = p.getZ()-1.1;
        inp<<"WWTP"<<i<<"\t"<< z <<"\tFREE\tNO\n";
    }

}
void VibeSWMM::writeConduits(std::fstream &inp) {
    inp<<"\n";
    inp<<"[CONDUITS]\n";
    inp<<";;Name	Node1	Node2	Length	N	Z1	Z2\n";
    inp<<";;============================================================================\n";

    std::vector<std::string> ConduitNames;
    ConduitNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierConduit,this->Network->getEdgeNames());

    int counter = 0;
    foreach(std::string name, ConduitNames) {
        Edge link = this->Network->getEdges(name)[0];
        std::vector<Point> points = this->Network->getPoints(name);


        Point nStartNode = points[link.getID1()];
        Point nEndNode = points[link.getID2()];


        double x = nStartNode.x - nEndNode.x;
        double y = nStartNode.y - nEndNode.y;

        double length = sqrt(x*x +y*y);


        int EndNode = this->findNewID(nStartNode);
        int StartNode = this->findNewID(nEndNode);
        double diameter =0;// 4-this->Network->getAttributes(name).getAttribute("Diameter")/1000;
        if (EndNode != -1 && StartNode != -1 && EndNode != StartNode)
            inp<<"LINK"<<counter<<"\tNODE"<<EndNode<<"\tNODE"<<StartNode<<"\t"<<length<<"\t"<<"0.01	" << diameter  <<"\t"	<< diameter << "\n";

        counter++;





    }
    std::vector<std::string> WWTPNames;
    WWTPNames = VectorDataHelper::findElementsWithIdentifier("WWTPConduit",this->Network->getEdgeNames());
    counter = 0;
    foreach(std::string name, WWTPNames) {
        Edge link = this->Network->getEdges(name)[0];
        std::vector<Point> points = this->Network->getPoints(name);


        Point nStartNode = points[link.getID1()];
        Point nEndNode = points[link.getID2()];


        double x = nStartNode.x - nEndNode.x;
        double y = nStartNode.y - nEndNode.y;

        double length = sqrt(x*x +y*y);


        int EndNode = this->findNewID(nStartNode);

        inp<<"WWTPConduit"<<counter<<"\tNODE"<<EndNode<<"\tWWTP"<<counter<<"\t"<<length<<"\t"<<"0.01	0	0\n";

        counter++;

    }
}
void VibeSWMM::writeLID_Controlls(std::fstream &inp) {

    std::vector<std::string> InletNames;
    InletNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierInlet,this->Network->getPointsNames());

    inp << "\n";
    inp << "[LID_CONTROLS]" << "\n";
    inp << ";;               Type/Layer Parameters" << "\n";
    inp << ";;-------------- ---------- ----------" << "\n";

    foreach(std::string name, InletNames) {
        Attribute inlet_attr;
        inlet_attr = this->Network->getAttributes(name);
        std::vector<Point> points = this->Network->getPoints(name);

        std::string id_catchment;
        id_catchment = QString::number( (int) inlet_attr.getAttribute(this->IdentifierCatchment+"ID") ).toStdString();

        Attribute catchment_attr = this->Catchments->getAttributes(this->IdentifierCatchment+id_catchment);

        double area = catchment_attr.getAttribute("Area");// node->area/10000.;

        if ((int)catchment_attr.getAttribute("NumberOfInfiltrationTrenches") == 0 || area == 0)
            continue;
        /**  [LID_CONTROLS]
    ;;               Type/Layer Parameters
    ;;-------------- ---------- ----------
    Infitration      IT
    Infitration      SURFACE    depth        vegetataive cover       surface roughness       surface slope           5
    Infitration      STORAGE    height        void ratio       conductivity         clogging factor
    Infitration      DRAIN      Drain Coefficient          Crain Exponent        Drain Offset Height          6  */

        inp << "Infiltration"<< id_catchment << "\t" << "IT" <<  "\n";
        inp << "Infiltration" << id_catchment<< "\t"   << "SURFACE" <<    "\t" <<  catchment_attr.getAttribute("h")*1000<<    "\t"     <<   "0.0"  <<    "\t"  <<   "0.1"<<    "\t" <<      fabs( catchment_attr.getAttribute("Gradient") )* 100 <<    "\t" <<      "5" << "\n";
        inp << "Infiltration" << id_catchment<< "\t"   << "STORAGE" <<    "\t" <<  "200"<<    "\t"     <<   "0.75"  <<    "\t"  <<   catchment_attr.getAttribute("kf") * 1000<<    "\t" <<       "0" << "\n";
        inp << "Infiltration"<< id_catchment << "\t"   << "DRAIN" <<    "\t" <<  "0" <<    "\t"     <<   "0.5"  <<     "\t"<< "0"<< "\t"<< "6" << "\n";
    }
}
void VibeSWMM::writeLID_Usage(std::fstream &inp) {


    std::vector<std::string> InletNames;
    InletNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierInlet,this->Network->getPointsNames());

    inp << "\n";
    inp << "[LID_USAGE]" << "\n";
    foreach(std::string name, InletNames) {
        Attribute inlet_attr;
        inlet_attr = this->Network->getAttributes(name);
        std::vector<Point> points = this->Network->getPoints(name);

        std::string id_catchment;
        id_catchment = QString::number( (int) inlet_attr.getAttribute(this->IdentifierCatchment+"ID") ).toStdString();


        Attribute catchment_attr = this->Catchments->getAttributes(this->IdentifierCatchment+id_catchment);

        double imp = catchment_attr.getAttribute("Impervious")*100;
        double inf = catchment_attr.getAttribute("RoofAreaInfitrated")/(200*200)*100;

        double treated = inf/imp*100;
        double area = catchment_attr.getAttribute("Area");// node->area/10000.;


        if ((int)catchment_attr.getAttribute("NumberOfInfiltrationTrenches") == 0 || area == 0)
            continue;
        /*[LID_USAGE]
    ;;Subcatchment   LID Process      Number  Area       Width      InitSatur  FromImprv  ToPerv     Report File
    ;;-------------- ---------------- ------- ---------- ---------- ---------- ---------- ---------- -----------
    sub68            Infitration      1       40000.00   200        90         100        0          "report1.txt"*/
        inp << "\n";
        inp << "[LID_USAGE]" << "\n";
        inp << "sub" <<id_catchment<< "\t"  << "Infiltration"<< id_catchment <<    "\t" <<  catchment_attr.getAttribute("NumberOfInfiltrationTrenches") <<    "\t"     <<   catchment_attr.getAttribute("As")   <<    "\t"  <<   "1"<<    "\t" <<       "0" <<    "\t" <<      treated <<    "\t" <<    "0" <<    "\n" ; //<< "\"report"<< id_catchment << ".txt\"" << "\n";
    }
}

void VibeSWMM::writeXSection(std::fstream &inp) {
    std::vector<std::string> OutfallNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierOutfall,this->Network->getPointsNames());
    std::vector<std::string> ConduitNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierConduit,this->Network->getPointsNames());
    std::vector<std::string> WWTPNames;
    WWTPNames = VectorDataHelper::findElementsWithIdentifier("WWTPConduit",this->Network->getEdgeNames());
    //XSection
    inp<<"\n";
    inp<<"[XSECTIONS]\n";
    inp<<";;Link	Type	G1	G2	G3	G4\n";
    inp<<";;==========================================\n";

    int counter = 0;
    foreach(std::string name, ConduitNames) {
        Attribute attr = this->Network->getAttributes(name);
        Edge link = this->Network->getEdges(name)[0];
        std::vector<Point> points = this->Network->getPoints(name);


        Point nStartNode = points[link.getID1()];
        Point nEndNode = points[link.getID2()];

        int EndNode = this->findNewID(nStartNode);
        int StartNode = this->findNewID(nEndNode);

        double d = attr.getAttribute("Diameter")/1000;
        if (EndNode != -1 && StartNode != -1 && EndNode != StartNode)
            inp << "LINK" <<counter<< "\tCIRCULAR\t"<< d <<" \t0\t0\t0\n";
        counter++;

    }

    for (int i = 0; i < OutfallNames.size(); i++) {
        std::string name = OutfallNames[i];

        inp << "WEIR" <<i<< "\tRECT_OPEN\t"<< "10" <<" \t6\t0\t0\n";
        counter++;

    }

    for (int i = 0; i < WWTPNames.size(); i++) {
        std::string name = WWTPNames[i];
        Attribute attr = this->Network->getAttributes(name);
        double d = attr.getAttribute("Diameter")/1000;
        inp << "WWTPConduit" <<i<< "\tCIRCULAR\t"<< d <<" \t0\t0\t0\n";
        counter++;

    }

    inp<<"\n";
}


void VibeSWMM::writeWeir(std::fstream &inp)
{

    inp<<"\n";
    inp<<"[WEIRS]\n";
    inp<<";;               Inlet            Outlet           Weir         Crest      Disch.     Flap End      End \n";
    inp<<";;Name           Node             Node             Type         Height     Coeff.     Gate Con.     Coeff.\n";
    inp<<";;-------------- ---------------- ---------------- ------------ ---------- ---------- ---- -------- ----------\n";
    //LINK984          NODE109          NODE985          TRANSVERSE   0          1.80       NO   0        0
    std::vector<std::string> namesWeir = VectorDataHelper::findElementsWithIdentifier(this->IdentifierWier, this->Network->getEdgeNames());
    for ( int i = 0; i < namesWeir.size(); i++ ) {
        std::vector<Point> WeirPoints = this->Network->getPoints(namesWeir[i]);


        double diameter = 0;
        bool storage = false;
        /*if (nStartNode->Code == STORAGE )
             storage = true;
         if (nEndNode->Code == STORAGE )
             storage = true;*/
        //Get Upper Points Connected with the  Weir
        std::vector<std::string> connectedConduits = VectorDataHelper::findConnectedEges((*this->Network), WeirPoints[0], 1, BOTH, this->IdentifierConduit);

        foreach (std::string s, connectedConduits ) {
            double d = this->Network->getAttributes(s).getAttribute("Diameter");
            if ( diameter < d ) {
                diameter = d;

            }
        }

        double x =  WeirPoints[0].x -  WeirPoints[1].x;
        double y = WeirPoints[0].y -  WeirPoints[1].y;
        int StartNode = this->findNewID(WeirPoints[0]);
        double lenght = sqrt(x*x +y*y);
        //if ( link->Code == LINK_OUTFALL ) {
        inp<<"WEIR"<<i<<"\tNODE"<<StartNode<<"\tOUTFALL"<<i<<"\t";
        inp<<"TRANSVERSE" << "\t";
        if (storage == true ) {
            inp<< diameter*0.6/1000. << "\t";
        } else {
            inp<< diameter*0.85/1000. << "\t";
        }
        inp<<"1.8" << "\t";
        inp<<"NO" << "\t";
        inp<<"0" << "\t";
        inp<<"0" << "\t" << "\n";

        // }
    }
}
void VibeSWMM::writeCoordinates(std::fstream &inp)
{
    std::vector<std::string> WWTPs = VectorDataHelper::findElementsWithIdentifier("WWTP_",this->Network->getPointsNames());
    std::vector<std::string> OutfallNames = VectorDataHelper::findElementsWithIdentifier(this->IdentifierOutfall,this->Network->getPointsNames());
    //COORDINATES
    //-------------------------//
    inp << "\n";
    inp<<"[COORDINATES]\n";
    inp<<";;Node\tXcoord\tyCoord\n";
    inp<<";;============================================================================\n";


    for ( int i = 0; i < this->PointList.size(); i++ ) {
        Point node = this->PointList[i];
        double x = node.x;
        double y = node.y;


        int id = i;
        inp << "NODE" << id ;
        inp << "\t";
        inp << x;
        inp << "\t";
        inp << y;
        inp << "\n";

    }

    for ( int i = 0; i < OutfallNames.size(); i++ ) {
        Point node = this->Network->getPoints(OutfallNames[i])[0];
        double x = node.x;
        double y = node.y;

        int id = i;
        inp << "OUTFALL" << id ;
        inp << "\t";
        inp << x;
        inp << "\t";
        inp << y;
        inp << "\n";
    }

    for ( int i = 0; i < WWTPs.size(); i++ ) {
        Point node = this->Network->getPoints(WWTPs[i])[0];
        double x = node.x;
        double y = node.y;

        int id = i;
        inp << "WWTP" << id ;
        inp << "\t";
        inp << x;
        inp << "\t";
        inp << y;
        inp << "\n";
    }

}

void VibeSWMM::writeSWMMFile() {
    QString fileName = this->SWMMPath.absolutePath()+ "/"+ "swmm.inp";
    std::fstream inp;
    inp.open(fileName.toAscii(),ios::out);
    writeSWMMheader(inp);
    writeSubcatchments(inp);
    writeLID_Controlls(inp);
    writeLID_Usage(inp);
    writeJunctions(inp);
    writeOutfalls(inp);
    writeStorage(inp);
    writeConduits(inp);
    writeWeir(inp);
    writeXSection(inp);
    writeDWF(inp);
    writeCoordinates(inp);

    inp.close();

}

void VibeSWMM::writeSWMMheader(std::fstream &inp)
{
    inp<<"[TITLE]\n";
    inp<<"VIBe\n\n";
    //-------------------------//

    //OPTIONS
    //-------------------------//
    inp<<"[OPTIONS]\n";
    //inp<<"FLOW_UNITS LPS\n";
    //inp<<"FLOW_ROUTING DYNWAVE\n";
    //inp<<"START_DATE 1/1/2000\n";
    //inp<<"START_TIME 00:00\n";
    //inp<<"END_DATE 1/2/2000\n";
    //inp<<"END_TIME 00:00\n";
    //inp<<"WET_STEP 00:05:00\n";
    //inp<<"DRY_STEP 01:00:00\n";
    //inp<<"ROUTING_STEP 00:05:00\n";
    //inp<<"VARIABLE_STEP 0.99\n";
    //inp<<"REPORT_STEP  00:05:00\n";
    //inp<<"REPORT_START_DATE 1/1/2000\n";
    //inp<<"REPORT_START_TIME 00:00\n\n";
    inp<<"FLOW_UNITS\t\tLPS\n";
    inp<<"INFILTRATION\t\tHORTON\n";
    inp<<"FLOW_ROUTING\t\tDYNWAVE\n";
    inp<<"START_DATE\t\t1/1/2000\n";
    inp<<"START_TIME\t\t00:00\n";
    inp<<"END_DATE\t\t1/2/2000\n";
    inp<<"END_TIME\t\t00:00\n";
    inp<<"REPORT_START_DATE\t1/1/2000\n";
    inp<<"REPORT_START_TIME\t00:00\n";
    inp<<"SWEEP_START\t\t01/01\n";
    inp<<"SWEEP_END\t\t12/31\n";
    inp<<"DRY_DAYS\t\t0\n";
    inp<<"REPORT_STEP\t\t00:05:00\n";
    inp<<"WET_STEP\t\t00:01:00\n";
    inp<<"DRY_STEP\t\t00:01:00\n";
    inp<<"ROUTING_STEP\t\t0:00:20\n";
    inp<<"ALLOW_PONDING\t\tNO\n";
    inp<<"INERTIAL_DAMPING\tPARTIAL\n";
    inp<<"VARIABLE_STEP\t\t0.75\n";
    inp<<"LENGTHENING_STEP\t300\n";
    inp<<"MIN_SURFAREA\t\t0\n";
    inp<<"NORMAL_FLOW_LIMITED\tBOTH\n";
    inp<<"SKIP_STEADY_STATE\tNO\n";
    inp<<"IGNORE_RAINFALL\t\tNO\n";
    inp<<"FORCE_MAIN_EQUATION\tH-W\n";
    inp<<"LINK_OFFSETS\t\tDEPTH\n\n";
    //-------------------------//

    //REPORT
    //-------------------------//
    inp<<"[REPORT]\n";
    inp<<"INPUT NO\n";
    inp<<"CONTINUITY YES\n";
    inp<<"FLOWSTATS YES\n";
    inp<<"CONTROLS NO\n";
    inp<<"SUBCATCHMENTS NONE\n";
    inp<<"NODES NONE\n";
    inp<<"LINKS NONE\n\n";
    //-------------------------//

    //RAINFILE
    //-------------------------//
    inp<<"[RAINGAGES]\n";
    inp<<";;Name\tFormat\tInterval\tSCF\tDataSource\tSourceName\tunits\n";
    inp<<";;============================================================================\n";
    inp<<"RG01\tVOLUME\t0:05\t1.0\tFILE\t";
    //inp<< "rain.dat";
    inp<< this->SWMMPath.absolutePath().toStdString() + "/" + "rain.dat";
    //mag mich nicht
    //inp<<rainFile.toAscii();

    inp<<"\tSTA01 MM\n";
    inp<<"\n";
    //-------------------------//

}
bool VibeSWMM::checkPoint(const Point &p) {
    foreach (Point p1, this->PointList) {
        if (p.compare2d(p1))
            return true;
    }
    return false;

}

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
#ifndef VIBESWMM_H
#define VIBESWMM_H

#include "module.h"
#include <QDir>
using namespace vibens;
class VIBE_HELPER_DLL_EXPORT VibeSWMM : public  Module {
VIBe_DECLARE_NODE (VibeSWMM)

private:

    VectorData * Network;
VectorData * Catchments;
VectorData *Network_out;

std::string IdentifierInlet;
std::string IdentifierConduit;
std::string IdentifierEndPoint;
std::string IdentifierShaft;
std::string IdentifierCatchment;
std::string IdentifierOutfall;
std::string IdentifierWier;
std::string RainFile;
std::string FileName;
std::vector<Point> PointList;
std::vector<Edge> EdgeList;

double Vp;
double Vr;
double Vwwtp;

std::vector<Point> EndPointList;
QDir SWMMPath;
QFile reportFile;

void writeSWMMheader(std::fstream &inp);

double climateChangeFactor;

void writeSWMMFile();
void RunSWMM();
int findNewID(const Point & p);
bool checkPoint(const Point & p);
void readInReportFile();

void writeSubcatchments(std::fstream &inp);
void writeJunctions(std::fstream &inp);
void writeDWF(std::fstream &inp);
void writeStorage(std::fstream &inp);
void writeOutfalls(std::fstream &inp);
void writeConduits(std::fstream &inp);
void writeXSection(std::fstream &inp);
void writeWeir(std::fstream &inp);
void writeCoordinates(std::fstream &inp);
void writeLID_Controlls(std::fstream &inp);
void writeLID_Usage(std::fstream &inp);
void writeRainFile();
int years;
double counterRain;
public:
VibeSWMM();
void run();
};

#endif // VIBESWMM_H

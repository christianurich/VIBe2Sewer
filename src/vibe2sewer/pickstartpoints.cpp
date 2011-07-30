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
#include "pickstartpoints.h"
#include "vectordatahelper.h"


VIBe_DECLARE_NODE_NAME(PickStartPoints, Sewer)
PickStartPoints::PickStartPoints()
{
    this->IdentifierStartPoints = "Inlet_";
    this->IdentifierCatchment = "GRID_";

    this->addParameter("IdentifierStartPoints", VIBe2::STRING, &this->IdentifierStartPoints);
    this->addParameter("StartPoints", VIBe2::VECTORDATA_IN, &this->startpoints);
    this->addParameter("StratPointsOut", VIBe2::VECTORDATA_OUT, &this->startpoints_out);
    this->addParameter("RasterData", VIBe2::RASTERDATA_IN, &this->Landuse);
    year = 1980;

}
void PickStartPoints::run() {

    *(this->startpoints_out) = *(this->startpoints);
    double cellSize = this->Landuse->getCellSize();
    //Get Existing StartPoints and Set Attribute Placed to true
    std::vector<std::string> names = VectorDataHelper::findElementsWithIdentifier(this->IdentifierStartPoints, this->startpoints_out->getAttributeNames());
    std::vector<std::string> unusedStartPoints;
    foreach (std::string name, names) {
        Attribute attr = this->startpoints->getAttributes(name);

        //Possible New Start Points
        if ((int) attr.getAttribute("Used") == 0) {
            unusedStartPoints.push_back(name);
        }
        this->startpoints_out->setAttributes(name, attr);
    }
    //Only place Nodes if Landuse is cont or diccont
    int counter = 0;

    for (int i = 0; i < unusedStartPoints.size(); i++) {
        //Cacluate RasterData Coordinates
        Point p = this->startpoints->getPoints(unusedStartPoints[i])[0];
        Attribute attr_p = this->startpoints->getAttributes(unusedStartPoints[i]);
        int id = (int)attr_p.getAttribute("GRID_ID");

        stringstream ids;
        ids << this->IdentifierCatchment << id;
        Attribute attr = this->startpoints->getAttributes(ids.str());
        int landuse = this->Landuse->getValue(p.x/cellSize, p.y/cellSize);
        int y = (int)attr.getAttribute("scheduled_year");
        if (landuse == VIBe2::ContUrbanFabric || landuse == VIBe2::DisContUrbanFabric || attr.getAttribute("UnitsR") > 0) {
            if (year > y) {
                Attribute attr = this->startpoints_out->getAttributes(unusedStartPoints[i]);
                attr.setAttribute("New", 1);
                this->startpoints_out->setAttributes(unusedStartPoints[i], attr);
                counter++;
            }
        }
    }
    Logger(Debug) << year;
    year++;

}


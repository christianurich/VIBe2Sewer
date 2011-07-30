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
#include "selectinfiltration.h"
#include "vectordatahelper.h"

VIBe_DECLARE_NODE_NAME( SelectInfiltration, Sewer)

SelectInfiltration::SelectInfiltration()
{
    Identifier= "GRID_";
    PercentInfiltration = 50;

    this->addParameter("Identifier", VIBe2::STRING, &Identifier);
    this->addParameter("PercentInfiltration", VIBe2::DOUBLE, &PercentInfiltration);

    this->addParameter("CatchmentIn", VIBe2::VECTORDATA_IN, &this->CatchmentIn);
    this->addParameter("CatchmentOut", VIBe2::VECTORDATA_OUT, &this->CatchmentOut);

}

void SelectInfiltration::run() {
    *CatchmentOut = *CatchmentIn;
    std::vector<std::string> names = VectorDataHelper::findElementsWithIdentifier(Identifier,CatchmentIn->getAttributeNames());
    foreach(std::string name, names) {
        Attribute attr = CatchmentOut->getAttributes(name);
        if (attr.getAttribute("Landuse") != 2){
            double RoofArea = attr.getAttribute("RoofArea")*200*200*PercentInfiltration/100.;
            attr.setAttribute("RoofAreaInfitrated", RoofArea);
        }
        CatchmentOut->setAttributes(name, attr);
    }
}

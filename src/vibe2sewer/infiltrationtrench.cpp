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
#include "infiltrationtrench.h"
#include "vectordatahelper.h"
VIBe_DECLARE_NODE_NAME( InfiltrationTrench, Sewer)
InfiltrationTrench::InfiltrationTrench()
{
    /* self.Identifier = "GRID_"
    self.R = 200 #l/s.ha
    self.D = 10
    self.StandardRoofArea = 275.
    self.CatchmentIn = VectorDataIn()
    self.CatchmentOut = VectorDataIn()
    self.addParameter(self, "R", VIBe2.DOUBLE)
    self.addParameter(self, "D", VIBe2.DOUBLE)
    self.addParameter(self, "Identifier", VIBe2.STRING)
    self.addParameter(self, "CatchmentIn", VIBe2.VECTORDATA_IN)
    self.addParameter(self, "CatchmentOut", VIBe2.VECTORDATA_OUT)
    self.addParameter(self, "StandardRoofArea", VIBe2.DOUBLE)*/

    Identifier = "GRID_";
    R = 200;
    D = 10;
    StandardRoofArea = 275;
    this->addParameter("Identifier", VIBe2::STRING, &this->Identifier);
    this->addParameter("StandardRoofArea", VIBe2::DOUBLE, &this->StandardRoofArea);
    this->addParameter("R", VIBe2::DOUBLE, &this->R);
    this->addParameter("D", VIBe2::DOUBLE, &this->D);

    this->addParameter("CatchmentIn", VIBe2::VECTORDATA_IN, &this->CatchmentIn);
    this->addParameter("CatchmentOut", VIBe2::VECTORDATA_OUT, &this->CatchmentOut);


}

void InfiltrationTrench::run() {
    *(CatchmentOut) = *(CatchmentIn);


    std::vector<std::string> names = VectorDataHelper::findElementsWithIdentifier(Identifier, CatchmentOut->getAttributeNames());


    foreach(std::string name, names) {
        Attribute attr = CatchmentOut->getAttributes(name);
        double RoofArea = attr.getAttribute("RoofAreaInfitrated");
        double Qzu = StandardRoofArea * R *(1.e-7);//l/shs
        double kf = 1.e-4;
        double  h = 1;
        double D = this->D*60;
        /*
        Qs = 1/2*kf*As
        Verf = (Qzu - Qs)*D
        As*h = (Qzu - 1/2*kf*As)*D
        As = Qzu/(h/D + 1/2*kf)
        */

        double As = Qzu /(h/D+0.5*kf);
        int NumberOfInfitrationTrenches = 0;
        int nt = RoofArea/StandardRoofArea;
        if (nt  > 0)
            NumberOfInfitrationTrenches = int(nt  +1);
        attr.setAttribute("NumberOfInfiltrationTrenches",NumberOfInfitrationTrenches );
        attr.setAttribute("As", As);
        attr.setAttribute("kf", kf);
        attr.setAttribute("h", h);
        CatchmentOut->setAttributes(name, attr);
    }

}

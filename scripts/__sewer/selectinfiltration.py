"""
@file
@author  Chrisitan Urich <christian.urich@gmail.com>
@version 1.0
@section LICENSE

This file is part of VIBe2
Copyright (C) 2011  Christian Urich

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""

from pyvibe import *
import pyvibehelper
class SelectInfiltration(Module):
    def __init__(self):
        Module.__init__(self)
        self.Identifier = "GRID_"
        self.PercentInfiltration = 50.
        self.CatchmentIn = VectorDataIn()
        self.CatchmentOut = VectorDataIn()
        self.addParameter(self, "CatchmentIn", VIBe2.VECTORDATA_IN)
        self.addParameter(self, "CatchmentOut", VIBe2.VECTORDATA_OUT)
        self.addParameter(self, "PercentInfiltration", VIBe2.DOUBLE)

    def run(self):
        Catchment = self.CatchmentOut.getItem()
        Catchment.addVectorData(self.CatchmentIn.getItem())
        
        names = pyvibehelper.findElementsWithIdentifier(self.Identifier, Catchment.getAttributeNames())
        for i in range(len(names)):
            attr = Catchment.getAttributes(names[i]) 
            if (attr.getAttribute("Landuse") != 2):
                RoofArea = attr.getAttribute("RoofArea")*200*200*self.PercentInfiltration/100.
                attr.setAttribute("RoofAreaInfitrated", RoofArea)
            Catchment.setAttributes(names[i], attr)         

            

            

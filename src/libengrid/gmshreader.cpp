// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +                                                                      +
// + This file is part of enGrid.                                         +
// +                                                                      +
// + Copyright 2008-2014 enGits GmbH                                      +
// +                                                                      +
// + enGrid is free software: you can redistribute it and/or modify       +
// + it under the terms of the GNU General Public License as published by +
// + the Free Software Foundation, either version 3 of the License, or    +
// + (at your option) any later version.                                  +
// +                                                                      +
// + enGrid is distributed in the hope that it will be useful,            +
// + but WITHOUT ANY WARRANTY; without even the implied warranty of       +
// + MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        +
// + GNU General Public License for more details.                         +
// +                                                                      +
// + You should have received a copy of the GNU General Public License    +
// + along with enGrid. If not, see <http://www.gnu.org/licenses/>.       +
// +                                                                      +
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include "gmshreader.h"
#include "correctsurfaceorientation.h"

#include <QFileInfo>
#include "guimainwindow.h"

#include "physicalboundarycondition.h"    // Guillaume
#include "guieditboundaryconditions.h"
#include "guimainwindow.h" // Guillaume
#include <iostream>
#include <QMap>
void GmshReader::readAscii1(vtkUnstructuredGrid *m_Grid)
{
  vtkIdType Nnodes, Ncells;
  QFile file(getFileName());
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream f(&file);
  QString word;
  f >> word;
  if (word != "$NOD") EG_ERR_RETURN("$NOD expected");
  f >> Nnodes;
  EG_VTKSP(vtkUnstructuredGrid, ug);
  QVector<vtkIdType> idxmap(Nnodes + 1);
  QVector<vec3_t> x(Nnodes);
  for (vtkIdType i = 0; i < Nnodes; ++i) {
    int ir;
    f >> ir >> x[i][0] >> x[i][1] >> x[i][2];
    idxmap[ir] = i;
  }
  f >> word;
  if (word != "$ENDNOD") EG_ERR_RETURN("$ENDNOD expected");
  f >> word;
  if (word != "$ELM") EG_ERR_RETURN("$ELM expected");
  f >> Ncells;
  allocateGrid(ug, Ncells, Nnodes, false);
  for (vtkIdType i = 0; i < Nnodes; ++i) {
    ug->GetPoints()->SetPoint(i, x[i].data());
  }
  EG_VTKSP(vtkIntArray, cell_code);
  cell_code->SetName("cell_code");
  cell_code->SetNumberOfValues(Ncells);
  ug->GetCellData()->AddArray(cell_code);
  for (vtkIdType i = 0; i < Ncells; ++i) {
    f >> word;
    int elm_type, reg_phys;
    f >> elm_type;
    f >> reg_phys;
    f >> word;
    f >> word;
    cell_code->SetValue(i, reg_phys);
    vtkIdType pts[8];
    size_t node;
    if        (elm_type == 2) { // triangle
      for (vtkIdType j = 0; j < 3; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_TRIANGLE,3,pts);
    } else if (elm_type == 3) { // quad
      for (vtkIdType j = 0; j < 4; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_QUAD,4,pts);
    } else if (elm_type == 4) { // tetrahedron
      for (vtkIdType j = 0; j < 4; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      vtkIdType h = pts[0];
      pts[0] = pts[1];
      pts[1] = h;
      ug->InsertNextCell(VTK_TETRA,4,pts);
    } else if (elm_type == 5) { // hexhedron
      for (vtkIdType j = 0; j < 8; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_HEXAHEDRON,8,pts);
    } else if (elm_type == 6) { // prism/wedge
      for (vtkIdType j = 0; j < 3; ++j) {
        f >> node;
        pts[j+3] = idxmap[node];
      }
      for (vtkIdType j = 0; j < 3; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_WEDGE,6,pts);
    } else if (elm_type == 7) { // pyramid
      for (vtkIdType j = 0; j < 5; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_PYRAMID,5,pts);
    }
  }
  ug->GetCellData()->AddArray(cell_code);
  m_Grid->DeepCopy(ug);
}

void GmshReader::readAscii2(vtkUnstructuredGrid *m_Grid)
{
  vtkIdType Nnodes, Ncells;
  QFile file(getFileName());
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream f(&file);
  QString word;
  f >> word;
  if (word != "$MeshFormat") EG_ERR_RETURN("$MeshFormat expected");
  f >> word;
  f >> word;
  f >> word;
  f >> word;
  if (word != "$EndMeshFormat") EG_ERR_RETURN("$EndMeshFormat expected");

  f >> word;
  if (word != "$PhysicalNames") EG_ERR_RETURN("$PhysicalNames expected");
  int NphysicalGroups;
  f >> NphysicalGroups;
  QMap<int,BoundaryCondition> physicalMap ;
  for (int i = 0; i<NphysicalGroups; i++){
    int physicalType;
    int iphys;
    QString group;
    f >> physicalType >> iphys >> group;
    physicalMap[iphys].setName(group) ;
    //(*m_BcMap)[iphys].setName(group) ;
    // PhysicalBoundaryCondition pbc; //Guillaume
  }
  f >> word;
  if (word != "$EndPhysicalNames") EG_ERR_RETURN("$EndPhysicalNames expected");



  f >> word;
  if (word != "$Nodes") EG_ERR_RETURN("$Nodes expected");
  f >> Nnodes;
  EG_VTKSP(vtkUnstructuredGrid, ug);
  QVector<vtkIdType> idxmap(Nnodes + 1);
  QVector<vec3_t> x(Nnodes);
  for (vtkIdType i = 0; i < Nnodes; ++i) {
    int ir;
    f >> ir >> x[i][0] >> x[i][1] >> x[i][2];
    idxmap[ir] = i;
  }
  f >> word;
  if (word != "$EndNodes") EG_ERR_RETURN("$EndNodes expected");
  f >> word;
  if (word != "$Elements") EG_ERR_RETURN("$Elements expected");
  f >> Ncells;
  allocateGrid(ug, Ncells, Nnodes, false);
  for (vtkIdType i = 0; i < Nnodes; ++i) {
    ug->GetPoints()->SetPoint(i, x[i].data());
  }
  EG_VTKSP(vtkIntArray, cell_code);
  cell_code->SetName("cell_code");
  cell_code->SetNumberOfValues(Ncells);
  ug->GetCellData()->AddArray(cell_code);

  for (vtkIdType i = 0; i < Ncells; ++i) {
    f >> word;
    int elm_type, Ntags, bc;
    f >> elm_type;
    f >> Ntags;
    if (Ntags == 0) {
      bc = 1;
    } else {
      f >> bc;
      if (bc <= 0) {
        bc = 99;
      }
      for (int j = 1; j < Ntags; ++j) f >> word;
    }
    cell_code->SetValue(i, bc);
    vtkIdType pts[8];
    size_t node;
    if        (elm_type == 2) { // triangle
      for (vtkIdType j = 0; j < 3; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_TRIANGLE,3,pts);
    } else if (elm_type == 3) { // quad
      for (vtkIdType j = 0; j < 4; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_QUAD,4,pts);
    } else if (elm_type == 4) { // tetrahedron
      for (vtkIdType j = 0; j < 4; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_TETRA,4,pts);
    } else if (elm_type == 5) { // hexhedron
      for (vtkIdType j = 0; j < 8; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_HEXAHEDRON,8,pts);
    } else if (elm_type == 6) { // prism/wedge
      for (vtkIdType j = 0; j < 3; ++j) {
        f >> node;
        pts[j+3] = idxmap[node];
      }
      for (vtkIdType j = 0; j < 3; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_WEDGE,6,pts);
    } else if (elm_type == 7) { // pyramid
      for (vtkIdType j = 0; j < 5; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_PYRAMID,5,pts);
    }
  }
  ug->GetCellData()->AddArray(cell_code);
  m_Grid->DeepCopy(ug);
}

//void GmshReader::readAscii2BND(vtkUnstructuredGrid *m_Grid, QMap<int,BoundaryCondition> myBCMap)
QMap<int,BoundaryCondition> GmshReader::readAscii2BND(vtkUnstructuredGrid *m_Grid)
{
  vtkIdType Nnodes, Ncells;
  QFile file(getFileName());
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream f(&file);
  QString word;
  f >> word;
  if (word != "$MeshFormat") EG_ERR_RETURN("$MeshFormat expected");
  f >> word;
  f >> word;
  f >> word;
  f >> word;
  if (word != "$EndMeshFormat") EG_ERR_RETURN("$EndMeshFormat expected");

  f >> word;
  if (word != "$PhysicalNames") EG_ERR_RETURN("$PhysicalNames expected");
  int NphysicalGroups;
  f >> NphysicalGroups;
  QMap<int,BoundaryCondition> physicalMap ;
  for (int i = 0; i<NphysicalGroups; i++){
    int physicalType;
    int iphys;
    QString group;
    f >> physicalType >> iphys >> group;
    if (physicalType ==2) {
        physicalMap[iphys].setName(group.replace("\"",""));
        cout << physicalMap[iphys].getName().toStdString() << endl;
        GuiMainWindow::pointer()->setBC(iphys, BoundaryCondition(physicalMap[iphys].getName(), "patch", -1));
    }
    //physicalMap[physicalMap->keys()[iphys]].setName(group.replace("\"","")) ;
    // PhysicalBoundaryCondition pbc; //Guillaume
  }
  f >> word;
  if (word != "$EndPhysicalNames") EG_ERR_RETURN("$EndPhysicalNames expected");



  f >> word;
  if (word != "$Nodes") EG_ERR_RETURN("$Nodes expected");
  f >> Nnodes;
  EG_VTKSP(vtkUnstructuredGrid, ug);
  QVector<vtkIdType> idxmap(Nnodes + 1);
  QVector<vec3_t> x(Nnodes);
  for (vtkIdType i = 0; i < Nnodes; ++i) {
    int ir;
    f >> ir >> x[i][0] >> x[i][1] >> x[i][2];
    idxmap[ir] = i;
  }
  f >> word;
  if (word != "$EndNodes") EG_ERR_RETURN("$EndNodes expected");
  f >> word;
  if (word != "$Elements") EG_ERR_RETURN("$Elements expected");
  f >> Ncells;
  allocateGrid(ug, Ncells, Nnodes, false);
  for (vtkIdType i = 0; i < Nnodes; ++i) {
    ug->GetPoints()->SetPoint(i, x[i].data());
  }
  EG_VTKSP(vtkIntArray, cell_code);
  cell_code->SetName("cell_code");
  cell_code->SetNumberOfValues(Ncells);
  ug->GetCellData()->AddArray(cell_code);

  for (vtkIdType i = 0; i < Ncells; ++i) {
    f >> word;
    int elm_type, Ntags, bc;
    f >> elm_type;
    f >> Ntags;
    if (Ntags == 0) {
      bc = 1;
    } else {
      f >> bc;
      if (bc <= 0) {
        bc = 99;
      }
      for (int j = 1; j < Ntags; ++j) f >> word;
    }
    cell_code->SetValue(i, bc);
    vtkIdType pts[8];
    size_t node;
    if        (elm_type == 2) { // triangle
      for (vtkIdType j = 0; j < 3; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_TRIANGLE,3,pts);
    } else if (elm_type == 3) { // quad
      for (vtkIdType j = 0; j < 4; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_QUAD,4,pts);
    } else if (elm_type == 4) { // tetrahedron
      for (vtkIdType j = 0; j < 4; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_TETRA,4,pts);
    } else if (elm_type == 5) { // hexhedron
      for (vtkIdType j = 0; j < 8; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_HEXAHEDRON,8,pts);
    } else if (elm_type == 6) { // prism/wedge
      for (vtkIdType j = 0; j < 3; ++j) {
        f >> node;
        pts[j+3] = idxmap[node];
      }
      for (vtkIdType j = 0; j < 3; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_WEDGE,6,pts);
    } else if (elm_type == 7) { // pyramid
      for (vtkIdType j = 0; j < 5; ++j) {
        f >> node;
        pts[j] = idxmap[node];
      }
      ug->InsertNextCell(VTK_PYRAMID,5,pts);
    }
  }
  ug->GetCellData()->AddArray(cell_code);
  m_Grid->DeepCopy(ug);
  return physicalMap;
}

void GmshReader::operate()
{
  try {
    QFileInfo file_info(GuiMainWindow::pointer()->getFilename());


    readInputFileName(file_info.completeBaseName() + ".msh");
    if (isValid()) {
      if (format == ascii1) {
        readAscii1(m_Grid);
      }
      if (format == ascii2) {
        readAscii2(m_Grid);
      }
      if (format == ascii2BND) {

          QMap<int,BoundaryCondition> myBCMap;
        myBCMap = readAscii2BND(m_Grid);
        //*m_BcMap = readAscii2BND(m_Grid);
        GuiEditBoundaryConditions editbcs;
        editbcs();
        editbcs.setBcMap(myBCMap);   //::setMap(myBCMap);
        //editbcs();
        //QMap<QString, PhysicalBoundaryCondition> m_PhysicalBoundaryConditionsMap;

        //foreach( int key, myBCMap.keys() ){
        //cout << key <<  myBCMap[key].getName().toStdString() << endl;
        //m_PhysicalBoundaryConditionsMap[myBCMap[key].getName()]=PhysicalBoundaryCondition();
        //m_PhysicalBoundaryConditionsMap[myBCMap[key].getName()].setName(myBCMap[key].getName());
        //m_PhysicalBoundaryConditionsMap[myBCMap[key].getName()].setType("wall");
        //m_PhysicalBoundaryConditionsMap[myBCMap[key].getName()].setIndex(key);
        //}
        //GuiMainWindow::pointer()->setAllPhysicalBoundaryConditions(m_PhysicalBoundaryConditionsMap);
      }
      createBasicFields(m_Grid, m_Grid->GetNumberOfCells(), m_Grid->GetNumberOfPoints());
      updateCellIndex(m_Grid);
      CorrectSurfaceOrientation corr_surf;
      corr_surf.setGrid(m_Grid);
      corr_surf();
    }
  } catch (Error err) {
    err.display();
  }
}

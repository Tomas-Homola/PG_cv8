#include "ImageViewer.h"
#include <iostream>

ImageViewer::ImageViewer(QWidget* parent) : QMainWindow(parent), ui(new Ui::ImageViewerClass)
{
	ui->setupUi(this);

	openNewTabForImg(new ViewerWidget("Default window", QSize(720, 600)));
	ui->tabWidget->setCurrentIndex(ui->tabWidget->count() - 1);
	getCurrentViewerWidget()->clear();

	// settings left
	ui->pushButton_Clear->setEnabled(false);
	ui->pushButton_Create->setEnabled(true);
	ui->groupBox_Divisions->setEnabled(true);
	ui->pushButton_Export->setEnabled(false);
	ui->pushButton_Import->setEnabled(true);

	camera.setClipNearDistance(ui->doubleSpinBox_ClipNear->value());
	camera.setClipFarDistance(ui->doubleSpinBox_ClipFar->value());

	// setings right
	lightSource.x = (double)ui->horizontalSlider_LightX->value();
	lightSource.y = (double)ui->horizontalSlider_LightY->value();
	lightSource.z = (double)ui->horizontalSlider_LightZ->value();
	lightSource.lightColor = QColor("#FFFFFF");
	ui->pushButton_LightColor->setStyleSheet("background-color:#FFFFFF");

	// difuzna zlozka
	coeficientsPOM.difuse.red = ui->doubleSpinBox_DiffuseRed->value();
	coeficientsPOM.difuse.green = ui->doubleSpinBox_DiffuseGreen->value();
	coeficientsPOM.difuse.blue = ui->doubleSpinBox_DiffuseBlue->value();

	// odrazova zlozka
	coeficientsPOM.specular.red = ui->doubleSpinBox_SpecularRed->value();
	coeficientsPOM.specular.green = ui->doubleSpinBox_SpecularGreen->value();
	coeficientsPOM.specular.blue = ui->doubleSpinBox_SpecularBlue->value();
	coeficientsPOM.specular.specularSharpness = ui->doubleSpinBox_SpecularSharpness->value(); // ostrost zrkadloveho odrazu

	//ambientna zlozka
	coeficientsPOM.ambient.red = ui->doubleSpinBox_AmbientRed->value();
	coeficientsPOM.ambient.green = ui->doubleSpinBox_AmbientGreen->value();
	coeficientsPOM.ambient.blue = ui->doubleSpinBox_AmbientBlue->value();
	coeficientsPOM.ambient.ambientColor = QColor("#FFFFFF");
	ui->pushButton_AmbientColor->setStyleSheet("background-color:#FFFFFF");

}

void ImageViewer::infoMessage(QString message)
{
	msgBox.setWindowTitle(QString("Info message"));
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setText(message);
	msgBox.exec();
}
void ImageViewer::warningMessage(QString message)
{
	msgBox.setWindowTitle(QString("Warning message"));
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setText(message);
	msgBox.exec();
}

bool ImageViewer::exportOctahedron()
{
	QList<Vertex*>* vertices = octahedron.pointerVertices();
	QList<H_edge*>* edges = octahedron.pointerEdges();
	QList<Face*>* faces = octahedron.pointerFaces();

	QString vtkFileName = QFileDialog::getSaveFileName(this, "Save File", "octasphere", tr("Vtk File (*.vtk)"));

	qDebug() << "filename:" << vtkFileName;

	QFile vtkFile(vtkFileName);
	if (!vtkFile.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;

	QTextStream toFile(&vtkFile);

	toFile << "# vtk DataFile Version 3.0\nOctasphere example\nASCII\nDATASET POLYDATA\n";

	qDebug() << "hlavicka done";

	// vrcholy - hlavicka
	toFile << "POINTS " << vertices->size() << " double\n";

	// suradnice vrcholov
	for (int i = 0; i < vertices->size(); i++)
		toFile << (*vertices)[i]->vertexInfo() << "\n";

	qDebug() << "points done";
	
	// hrany - hlavicka
	toFile << "LINES " << edges->size() / 2 << " " << (edges->size() / 2) * 3 << "\n";
	
	// hrany info
	for (int i = 0; i < edges->size(); i++)
	{
		//qDebug() << QString("edges[%1]").arg(i) << (*edges)[i]->edgeVerticesInfo() << "\n";
		if ((*edges)[i]->getVertexOrigin()->getIndex() > (*edges)[i]->getEdgePair()->getVertexOrigin()->getIndex())
			toFile << "2 " << (*edges)[i]->edgeVerticesInfo() << "\n";
	}

	qDebug() << "lines done";
	
	// steny - hlavicka
	toFile << "POLYGONS " << faces->size() << " " << faces->size() * 4 << "\n";
	for (int i = 0; i < faces->size(); i++)
		toFile << "3 " << (*faces)[i]->faceVerticesInfo() << "\n";

	qDebug() << "polygons done";
	

	vtkFile.close();

	return true;
}
bool ImageViewer::importOctahedron()
{
	QList<Vertex*>* vertices = new QList<Vertex*>;
	QList<H_edge*>* edges = new QList<H_edge*>;
	QList<Face*>* faces = new QList<Face*>;

	QString line = "";
	int numOfVertices = 0, numOfLines = 0, numOfFaces = 0;
	int vertexStartIndex = -1, vertexEndIndex = -1;
	int index1 = -1, index2 = -1, index3 = -1;
	double x = 0.0, y = 0.0, z = 0.0;

	H_edge* edge1 = nullptr, * edge2 = nullptr, * edge3 = nullptr;
	Face* newFace = nullptr;

	QString vtkFileName = QFileDialog::getOpenFileName(this, "Open File", "", tr("Vtk File (*.vtk)"));
	if (vtkFileName.isEmpty())
	{
		warningMessage("Empty file name");
		return false;
	}

	QFile vtkFile(vtkFileName);
	if (!vtkFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		warningMessage(QString("Error with opening file"));
		return false;
	}
	else
		qDebug() << "File opened";

	QTextStream fromfile(&vtkFile);
	
	// Kontrola prvej hlavicky
	if ((line = fromfile.readLine()) != QString("# vtk DataFile Version 3.0"))
	{
		warningMessage("Incorrect first header");
		return false;
	}
	else
		qDebug() << "First header OK";

	// vtk file description
	line = fromfile.readLine();
	qDebug() << "vtk file desription:" << line;

	// Kontrola ASCII
	if ((line = fromfile.readLine()) != QString("ASCII"))
	{
		warningMessage("Other than ASCII written");
		return false;
	}
	else
		qDebug() << "ASCII OK";

	// Kontrola DATASET POLYDATA
	if ((line = fromfile.readLine()) != QString("DATASET POLYDATA"))
	{
		warningMessage("Other than DATASET POLYDATA	written");
		return false;
	}
	else
		qDebug() << "DATASET POLYDATA OK";

	// nacitavanie POINTS
	line = fromfile.readLine();

	if (line.split(" ").at(0) != QString("POINTS"))
	{
		warningMessage("Other than 'POINTS' written");
		return false;
	}
	else
		qDebug() << "'POINTS' OK";

	numOfVertices = line.split(" ").at(1).toInt();

	qDebug() << "numOfVertices:" << numOfVertices;

	for (int i = 0; i < numOfVertices; i++)
	{
		line = fromfile.readLine().trimmed();

		x = line.split(" ").at(0).toDouble();
		y = line.split(" ").at(1).toDouble();
		z = line.split(" ").at(2).toDouble();

		(*vertices).append(new Vertex(x, y, z, i));
	}

	// nacitavanie LINES (H_edge)
	line = fromfile.readLine();

	if (line.split(" ").at(0) != QString("LINES"))
	{
		warningMessage("Other than 'LINES' written");

		for (int i = 0; i < (*vertices).size(); i++)
			delete (*vertices)[i];

		delete vertices;

		return false;
	}
	else
		qDebug() << "'LINES' OK";

	numOfLines = line.split(" ").at(1).toInt();

	for (int i = 0; i < numOfLines; i++)
	{
		line = fromfile.readLine().trimmed();

		if (line.split(" ").at(0) != "2")
		{
			warningMessage("Error with loading 'LINES'");
			
			for (int j = 0; j < (*vertices).size(); j++)
				delete (*vertices)[j];

			for (int j = 0; j < (*edges).size(); j++)
				delete (*edges)[j];

			delete vertices;
			delete edges;

			return false;
		}

		vertexStartIndex = line.split(" ").at(1).toInt();
		vertexEndIndex = line.split(" ").at(2).toInt();

		edge1 = new H_edge(); // nacitana hrana
		edge2 = new H_edge(); // plus k nej este parova hrana, ktora v subore nie je zapisana

		// nastavenie veci, co vlastne pozname: body hrany a jej par
		edge1->setVertexOrigin((*vertices)[vertexStartIndex]);
		edge1->setVertexEnd((*vertices)[vertexEndIndex]);
		edge1->setEdgePair(edge2);

		edge2->setVertexOrigin((*vertices)[vertexEndIndex]);
		edge2->setVertexEnd((*vertices)[vertexStartIndex]);
		edge2->setEdgePair(edge1);

		// doplnenie hrany ku nacitanym bodom
		if (!(*vertices)[vertexStartIndex]->hasEdge())
			(*vertices)[vertexStartIndex]->setEdge(edge1);
		
		if (!(*vertices)[vertexEndIndex]->hasEdge())
			(*vertices)[vertexEndIndex]->setEdge(edge2);

		(*edges).append(edge1);
		(*edges).append(edge2);
	}

	for (int i = 0; i < edges->size(); i++)
	{
		qDebug() << QString("edges[%1]: %2 %3").arg(i).arg((*edges)[i]->getVertexOriginIndex()).arg((*edges)[i]->getVert_EndIndex());
		qDebug() << QString("edges[%1] pair: %2 %3").arg(i).arg((*edges)[i]->getEdgePair()->getVertexOriginIndex()).arg((*edges)[i]->getEdgePair()->getVert_EndIndex()) << "\n";
	}

	qDebug() << "'LINES' done";

	// nacitanie POLYGONS
	line = fromfile.readLine();

	if (line.split(" ").at(0) != QString("POLYGONS"))
	{
		warningMessage("Other than 'POLYGONS' written");
		
		for (int j = 0; j < (*vertices).size(); j++)
			delete (*vertices)[j];

		for (int j = 0; j < (*edges).size(); j++)
			delete (*edges)[j];

		delete vertices;
		delete edges;
		
		return false;
	}
	else
		qDebug() << "'POLYGONS OK'";

	numOfFaces = line.split(" ").at(1).toInt();

	qDebug() << "numOfFaces:" << numOfFaces;

	for (int i = 0; i < numOfFaces; i++)
	{
		line = fromfile.readLine().trimmed();

		if ((line.split(" ").at(0).toInt() != 3) || (line.split(" ").size() != 4))
		{
			warningMessage("Error with loading 'POLYGONS'");
			
			for (int j = 0; j < (*vertices).size(); j++)
				delete (*vertices)[j];

			for (int j = 0; j < (*edges).size(); j++)
				delete (*edges)[j];

			for (int j = 0; j < (*faces).size(); j++)
				delete (*faces)[j];

			delete vertices;
			delete edges;
			delete faces;
			
			return false;
		}

		edge1 = nullptr; edge2 = nullptr; edge3 = nullptr;

		index1 = line.split(" ").at(1).toInt();
		index2 = line.split(" ").at(2).toInt();
		index3 = line.split(" ").at(3).toInt();

		for (int j = 0; j < edges->size(); j++)
		{
			if (((*edges)[j]->getVertexOriginIndex() == index1) && ((*edges)[j]->getVert_EndIndex() == index2))
			{
				edge1 = (*edges)[j]; // prva hrana plosky
				
				if (edge1 != nullptr && edge2 != nullptr && edge3 != nullptr)
				{
					qDebug() << QString("All edges for face[%1] found").arg(i);
					break;
				}
				else
					continue;
			}
			else if (((*edges)[j]->getVertexOriginIndex() == index2) && ((*edges)[j]->getVert_EndIndex() == index3))
			{
				edge2 = (*edges)[j]; // druha hrana plosky
				
				if (edge1 != nullptr && edge2 != nullptr && edge3 != nullptr)
				{
					qDebug() << QString("All edges for face[%1] found").arg(i);
					break;
				}
				else
					continue;
			}
			else if (((*edges)[j]->getVertexOriginIndex() == index3) && ((*edges)[j]->getVert_EndIndex() == index1))
			{
				edge3 = (*edges)[j]; // tretia hrana plosky
				
				if (edge1 != nullptr && edge2 != nullptr && edge3 != nullptr)
				{
					qDebug() << QString("All edges for face[%1] found").arg(i);
					break;
				}
				else
					continue;
			}
		}

		newFace = new Face();

		newFace->setEdge(edge1);

		edge1->setEdgeNext(edge2); edge1->setEdgePrevious(edge3); edge1->setFace(newFace);
		edge2->setEdgeNext(edge3); edge2->setEdgePrevious(edge1); edge2->setFace(newFace);
		edge3->setEdgeNext(edge1); edge3->setEdgePrevious(edge2); edge3->setFace(newFace);

		(*faces).append(newFace);	
	}

	qDebug() << "'POLYGONS done'";

	octahedron.clear();
	octahedron.getVertices(vertices);
	octahedron.getEdges(edges);
	octahedron.getFaces(faces);

	// vypis veci ako kontrola po importe
	/*
	for (int i = 0; i < octahedron.Vertices().size(); i++)
	{
		if (octahedron.Vertices()[i]->hasEdge())
			qDebug() << QString("vertices[%1] has edge").arg(i);
		else
			qDebug() << QString("vertices[%1] has NO edge").arg(i);

		qDebug() << QString("vertices[%1]: ").arg(i) + octahedron.Vertices()[i]->vertexInfo();
	}

	for (int i = 0; i < octahedron.Edges().size(); i++)
	{
		if (octahedron.Edges()[i]->hasVertexOrigin())
			qDebug() << QString("edges[%1] has vert_origin").arg(i);
		else
			qDebug() << QString("edges[%1] has NO vert_origin").arg(i);

		if (octahedron.Edges()[i]->hasEdgeNext())
			qDebug() << QString("edges[%1] has edge_next").arg(i);
		else
			qDebug() << QString("edges[%1] has NO edge_next").arg(i);

		if (octahedron.Edges()[i]->hasEdgePrevious())
			qDebug() << QString("edges[%1] has edge_prev").arg(i);
		else
			qDebug() << QString("edges[%1] has NO edge_prev").arg(i);

		if (octahedron.Edges()[i]->hasPair())
			qDebug() << QString("edges[%1] has pair").arg(i);
		else
			qDebug() << QString("edges[%1] has NO pair").arg(i);

		if (octahedron.Edges()[i]->hasFace())
			qDebug() << QString("edges[%1] has face").arg(i) << "\n";
		else
			qDebug() << QString("edges[%1] has NO face").arg(i) << "\n";
	}

	for (int i = 0; i < octahedron.Faces().size(); i++)
		qDebug() << QString("faces[%1]: ") + octahedron.Faces()[i]->faceVerticesInfo();*/

	return true;
}

void ImageViewer::subdivideOctahedron(int divisions)
{
	// smerniky na QListy vrcholov, hran a plosok octahedronu/octasfery
	QList<Vertex*>* vertices = octahedron.pointerVertices();
	QList<H_edge*>* edges = octahedron.pointerEdges();
	QList<Face*>* faces = octahedron.pointerFaces();

	int originalNumOfFaces = 0;
	int originalNumOfEdges = 0;
	int originalNumOfVertices = 0;
	double xStart = 0.0, xEnd = 0.0, xMiddle = 0.0;
	double yStart = 0.0, yEnd = 0.0, yMiddle = 0.0;
	double zStart = 0.0, zEnd = 0.0, zMiddle = 0.0;
	bool alreadyThere = false;

	Vertex* newVertex1 = nullptr; Vertex* newVertex2 = nullptr; Vertex* newVertex3 = nullptr;
	H_edge* edge = nullptr; H_edge* edge_next = nullptr; H_edge* edge_prev = nullptr;
	// smerniky na nove hrany po deleni
	H_edge* e0 = nullptr, * e1 = nullptr, * e2 = nullptr, // pre stenu A
		  * e3 = nullptr, * e4 = nullptr, * e5 = nullptr, // pre stenu B
		  * e6 = nullptr, * e7 = nullptr, * e8 = nullptr, // pre stenu C
		  * e9 = nullptr, * e10 = nullptr, * e11 = nullptr; // pre stenu D
	// smerniky na nove steny
	Face* A = nullptr, * B = nullptr, * C = nullptr, * D = nullptr;

	for (int i = 0; i < divisions; i++)
	{
		originalNumOfFaces = octahedron.numOfFaces();
		originalNumOfEdges = octahedron.numOfEdges();
		originalNumOfVertices = octahedron.numOfVertices();

		for (int j = 0; j < originalNumOfFaces; j++)
		{
			// priradenie smernikov hran plosky, aby som nemusel vela vypisovat
			edge = (*faces)[j]->getEdge();
			edge_next = (*faces)[j]->getEdge()->getEdgeNext();
			edge_prev = (*faces)[j]->getEdge()->getEdgePrevious();

			// bod #1
			// x suradnica
			xStart = edge->getVertexOrigin()->getX();
			xEnd = edge_next->getVertexOrigin()->getX();
			xMiddle = (xStart + xEnd) / 2.0;

			// y suradnica
			yStart = edge->getVertexOrigin()->getY();
			yEnd = edge_next->getVertexOrigin()->getY();
			yMiddle = (yStart + yEnd) / 2.0;

			// z suradnica
			zStart = edge->getVertexOrigin()->getZ();
			zEnd = edge_next->getVertexOrigin()->getZ();
			zMiddle = (zStart + zEnd) / 2.0;

			newVertex1 = new Vertex(xMiddle, yMiddle, zMiddle, vertices->size());

			// moze sa stat, ze uz napr. na druhej stene sa moze vypocitat novy bod taky, ze uz existuje
			// hladat to zacne az v tej casti, kde by mali byt tie nove vrcholy, snad sa tym skrati cas trochu
			for (int k = originalNumOfVertices; k < vertices->size(); k++)
			{
				if ((*newVertex1) == (*(*vertices)[k])) // dereferencie z Vertex* na Vertex, aby fungovalo ==
				{
					delete newVertex1; // vymaze novy vypocitany
					newVertex1 = (*vertices)[k]; // vyberie sa ten, co uz bol predtym vypocitany
					alreadyThere = true;
					break;
				}
			}

			if (!alreadyThere) // ak teda ide o novy vrchol, tak sa prida
				vertices->append(newVertex1);

			alreadyThere = false;

			// bod #2
			// x suradnica
			xStart = edge_next->getVertexOrigin()->getX();
			xEnd = edge_prev->getVertexOrigin()->getX();
			xMiddle = (xStart + xEnd) / 2.0;

			// y suradnica
			yStart = edge_next->getVertexOrigin()->getY();
			yEnd = edge_prev->getVertexOrigin()->getY();
			yMiddle = (yStart + yEnd) / 2.0;

			// z suradnica
			zStart = edge_next->getVertexOrigin()->getZ();
			zEnd = edge_prev->getVertexOrigin()->getZ();
			zMiddle = (zStart + zEnd) / 2.0;

			newVertex2 = new Vertex(xMiddle, yMiddle, zMiddle, vertices->size());

			// moze sa stat, ze uz napr. na druhej stene sa moze vypocitat novy bod taky, ze uz existuje
			// hladat to zacne az v tej casti, kde by mali byt tie nove vrcholy, snad sa tym skrati cas trochu
			for (int k = originalNumOfVertices; k < vertices->size(); k++)
			{
				if ((*newVertex2) == (*(*vertices)[k])) // dereferencie z Vertex* na Vertex, aby fungovalo ==
				{
					delete newVertex2; // vymaze novy vypocitany
					newVertex2 = (*vertices)[k]; // vyberie sa ten, co uz bol predtym vypocitany
					alreadyThere = true;
					break;
				}
			}

			if (!alreadyThere) // ak teda ide o novy vrchol, tak sa prida
				vertices->append(newVertex2);

			alreadyThere = false;

			// bod #3
			// x suradnica
			xStart = edge_prev->getVertexOrigin()->getX();
			xEnd = edge->getVertexOrigin()->getX();
			xMiddle = (xStart + xEnd) / 2.0;

			// y suradnica
			yStart = edge_prev->getVertexOrigin()->getY();
			yEnd = edge->getVertexOrigin()->getY();
			yMiddle = (yStart + yEnd) / 2.0;

			// z suradnica
			zStart = edge_prev->getVertexOrigin()->getZ();
			zEnd = edge->getVertexOrigin()->getZ();
			zMiddle = (zStart + zEnd) / 2.0;

			newVertex3 = new Vertex(xMiddle, yMiddle, zMiddle, vertices->size());

			// moze sa stat, ze uz napr. na druhej stene sa moze vypocitat novy bod taky, ze uz existuje
			// hladat to zacne az v tej casti, kde by mali byt tie nove vrcholy, snad sa tym skrati cas trochu
			for (int k = originalNumOfVertices; k < vertices->size(); k++)
			{
				if ((*newVertex3) == (*(*vertices)[k])) // dereferencie z Vertex* na Vertex, aby fungovalo ==
				{
					delete newVertex3; // vymaze novy vypocitany
					newVertex3 = (*vertices)[k]; // vyberie sa ten, co uz bol predtym vypocitany
					alreadyThere = true;
					break;
				}
			}

			if (!alreadyThere) // ak teda ide o novy vrchol, tak sa prida
				vertices->append(newVertex3);

			alreadyThere = false;

			// allokacia novych hran a stien
			e0 = new H_edge(); e1 = new H_edge(); e2 = new H_edge();
			e3 = new H_edge(); e4 = new H_edge(); e5 = new H_edge();
			e6 = new H_edge(); e7 = new H_edge(); e8 = new H_edge();
			e9 = new H_edge(); e10 = new H_edge(); e11 = new H_edge();

			A = new Face(); B = new Face(); C = new Face(); D = new Face();

			// ak je "nullptr", tak sa zatial nevie, aka bude parova hrana
			// stena A
			e0->setAll(edge->getVertexOrigin(), A, e2, e1, nullptr);
			e1->setAll(newVertex1, A, e0, e2, e3);
			e2->setAll(newVertex3, A, e1, e0, nullptr);
			A->setEdge(e0);

			// stena B
			e3->setAll(newVertex3, B, e5, e4, e1);
			e4->setAll(newVertex1, B, e3, e5, e8);
			e5->setAll(newVertex2, B, e4, e3, e9);
			B->setEdge(e3);

			// stena C
			e6->setAll(newVertex1, C, e8, e7, nullptr);
			e7->setAll(edge_next->getVertexOrigin(), C, e6, e8, nullptr);
			e8->setAll(newVertex2, C, e7, e6, e4);
			C->setEdge(e6);

			// stena D
			e9->setAll(newVertex3, D, e11, e10, e5);
			e10->setAll(newVertex2, D, e9, e11, nullptr);
			e11->setAll(edge_prev->getVertexOrigin(), D, e10, e9, nullptr);
			D->setEdge(e9);

			// priradenie novych hran ku novo vzniknutym bodom (midPointy medzi vrcholmi trojuholnika)
			newVertex1->setEdge(e1);
			newVertex2->setEdge(e5);
			newVertex3->setEdge(e3);

			// priradenie novych hran ku bodom vo vrcholoch trojuholnika
			edge->getVertexOrigin()->setEdge(e0);
			edge_next->getVertexOrigin()->setEdge(e7);
			edge_prev->getVertexOrigin()->setEdge(e11);

			// priradenie do octahedronu
			edges->append(e0); edges->append(e1); edges->append(e2);
			faces->append(A);

			edges->append(e3); edges->append(e4); edges->append(e5);
			faces->append(B);

			edges->append(e6); edges->append(e7); edges->append(e8);
			faces->append(C);

			edges->append(e9); edges->append(e10); edges->append(e11);
			faces->append(D);
		}

		// vymazanie starych hran a stien
		for (int j = 0; j < originalNumOfEdges; j++)
		{
			delete (*edges)[0]; // odstranenie z pamate
			edges->removeFirst(); // odstranenie s QListu
		}

		for (int j = 0; j < originalNumOfFaces; j++)
		{
			delete (*faces)[0];
			faces->removeFirst();
		}

		// najdenie parovych hran
		for (int j = 0; j < edges->size(); j++)
		{
			for (int k = 0; k < edges->size(); k++)
			{
				// origin prveho == endpoint druheho && endpoint prveho == origin druheho

				if (((*edges)[j]->getVertexOriginIndex() == (*edges)[k]->getVertexEndIndex()) && ((*edges)[j]->getVertexEndIndex() == (*edges)[k]->getVertexOriginIndex()))
				{
					(*edges)[j]->setEdgePair((*edges)[k]);
					(*edges)[k]->setEdgePair((*edges)[j]);
					break;
				}
			}
		}

		// projekcia na jednotkovu sferu
		for (int j = 0; j < vertices->size(); j++)
		{
			(*vertices)[j]->projectToUnitSphere();
		}

	}

	qDebug() << "subdivision done";
}

void ImageViewer::parallelProjection()
{
	ViewerWidget* vW = getCurrentViewerWidget();

	double xBefore = 0.0, yBefore = 0.0, zBefore = 0.0;
	double xAfter = 0.0, yAfter = 0.0, zAfter = 0.0;
	double x2D = 0.0, y2D = 0.0;
	double temp = 0.0;
	// (sX, sY) je stred obrazka
	double sX = (double)(vW->getImgWidth() / 2);
	double sY = (double)(vW->getImgHeight() / 2);
	int dx = ui->horizontalSlider_dx->value();
	int dy = ui->horizontalSlider_dy->value();
	double objectScale = ui->doubleSpinBox_ObjectScale->value() * 1.5;
	double distance = 0.0;

	double a = (double)camera.getVector_n().x();
	double b = (double)camera.getVector_n().y();
	double c = (double)camera.getVector_n().z();

	QVector3D u = camera.getVector_u();
	QVector3D v = camera.getVector_v();

	QList<Face*>* faces = octahedron.pointerFaces();
	H_edge* edge = nullptr, * edge_next = nullptr, * edge_prev = nullptr;

	QVector<QPointF> newPoints;
	QList<QColor> colors;
	double zCoords[3] = { 0.0,0.0,0.0 };

	newPoints.append(QPointF()); newPoints.append(QPointF()); newPoints.append(QPointF());

	vW->clear();

	for (int i = 0; i < faces->size(); i++)
	{
		edge = (*faces)[i]->getEdge();
		edge_next = (*faces)[i]->getEdge()->getEdgeNext();
		edge_prev = (*faces)[i]->getEdge()->getEdgePrevious();

		// prvy bod
		xBefore = edge->getVertexOrigin()->getX();
		yBefore = edge->getVertexOrigin()->getY();
		zBefore = edge->getVertexOrigin()->getZ();

		distance = qSqrt((xBefore - a) * (xBefore - a) + (yBefore - b) * (yBefore - b) + (zBefore - c) * (zBefore - c)) * camera.getScaleValue();
		if ((distance < camera.getClipNearDistance()) || (distance > camera.getClipFarDistance()))
			continue;

		temp = (a * xBefore + b * yBefore + c * zBefore) / (a * a + b * b + c * c);

		xAfter = xBefore - a * temp;
		yAfter = yBefore - b * temp;
		zAfter = zBefore - c * temp;

		//https://stackoverflow.com/questions/23472048/projecting-3d-points-to-2d-plane
		x2D = v.x() * xAfter + v.y() * yAfter + v.z() * zAfter;
		y2D = u.x() * xAfter + u.y() * yAfter + u.z() * zAfter;

		newPoints[0].setX(x2D * camera.getScaleValue() * objectScale + sX + dx);
		newPoints[0].setY(y2D * camera.getScaleValue() * objectScale + sY + dy);

		zCoords[0] = zAfter;

		// druhy bod
		xBefore = edge_next->getVertexOrigin()->getX();
		yBefore = edge_next->getVertexOrigin()->getY();
		zBefore = edge_next->getVertexOrigin()->getZ();

		distance = qSqrt((xBefore - a) * (xBefore - a) + (yBefore - b) * (yBefore - b) + (zBefore - c) * (zBefore - c)) * camera.getScaleValue();
		if ((distance < camera.getClipNearDistance()) || (distance > camera.getClipFarDistance()))
			continue;

		temp = (a * xBefore + b * yBefore + c * zBefore) / (a * a + b * b + c * c);

		xAfter = xBefore - a * temp;
		yAfter = yBefore - b * temp;
		zAfter = zBefore - c * temp;

		//https://stackoverflow.com/questions/23472048/projecting-3d-points-to-2d-plane
		x2D = v.x() * xAfter + v.y() * yAfter + v.z() * zAfter;
		y2D = u.x() * xAfter + u.y() * yAfter + u.z() * zAfter;

		newPoints[1].setX(x2D * camera.getScaleValue() * objectScale + sX + dx);
		newPoints[1].setY(y2D * camera.getScaleValue() * objectScale + sY + dy);

		zCoords[1] = zAfter;

		// treti bod
		xBefore = edge_prev->getVertexOrigin()->getX();
		yBefore = edge_prev->getVertexOrigin()->getY();
		zBefore = edge_prev->getVertexOrigin()->getZ();

		distance = qSqrt((xBefore - a) * (xBefore - a) + (yBefore - b) * (yBefore - b) + (zBefore - c) * (zBefore - c)) * camera.getScaleValue();
		if ((distance < camera.getClipNearDistance()) || (distance > camera.getClipFarDistance()))
			continue;

		temp = (a * xBefore + b * yBefore + c * zBefore) / (a * a + b * b + c * c);

		xAfter = xBefore - a * temp;
		yAfter = yBefore - b * temp;
		zAfter = zBefore - c * temp;

		//https://stackoverflow.com/questions/23472048/projecting-3d-points-to-2d-plane
		x2D = v.x() * xAfter + v.y() * yAfter + v.z() * zAfter;
		y2D = u.x() * xAfter + u.y() * yAfter + u.z() * zAfter;

		newPoints[2].setX(x2D * camera.getScaleValue() * objectScale + sX + dx);
		newPoints[2].setY(y2D * camera.getScaleValue() * objectScale + sY + dy);

		zCoords[2] = zAfter;

		colors.clear();
		colors.append(edge->getVertexOrigin()->getVertexColor());
		colors.append(edge_next->getVertexOrigin()->getVertexColor());
		colors.append(edge_prev->getVertexOrigin()->getVertexColor());

		//vW->createGeometry(newPoints, QColor("#FFFFFF"), QColor("#000000"), 0);
		//vW->drawPolygon(newPoints, QColor("#FFFFFF"));
		vW->drawPolygonT(newPoints, colors, zCoords, camera.getShadingType());
	}

}
void ImageViewer::perspectiveProjection()
{
	ViewerWidget* vW = getCurrentViewerWidget();

	double xBefore = 0.0, yBefore = 0.0, zBefore = 0.0;
	double xAfter = 0.0, yAfter = 0.0, zAfter = 0.0;
	double x2D = 0.0, y2D = 0.0;
	double temp = 0.0;
	// (midPointX, midPointY) je stred obrazka
	double midPointX = (double)(vW->getImgWidth() / 2);
	double midPointY = (double)(vW->getImgHeight() / 2);
	int dx = ui->horizontalSlider_dx->value();
	int dy = ui->horizontalSlider_dy->value();
	double objectScale = ui->doubleSpinBox_ObjectScale->value() * 1.5;
	double distance = 0.0;

	double a = (double)camera.getVector_n().x();
	double b = (double)camera.getVector_n().y();
	double c = (double)camera.getVector_n().z();

	QVector3D u = camera.getVector_u();
	QVector3D v = camera.getVector_v();
	QVector3D S = camera.getVector_n();

	S.setX(S.x() * camera.getCameraDistance());
	S.setY(S.y() * camera.getCameraDistance());
	S.setZ(S.z() * camera.getCameraDistance());

	QList<Face*>* faces = octahedron.pointerFaces();
	H_edge* edge = nullptr, * edge_next = nullptr, * edge_prev = nullptr;

	QVector<QPointF> newPoints;
	QList<QColor> colors;
	double zCoords[3] = { 0.0,0.0,0.0 };

	newPoints.append(QPointF()); newPoints.append(QPointF()); newPoints.append(QPointF());

	vW->clear();

	for (int i = 0; i < faces->size(); i++)
	{
		edge = (*faces)[i]->getEdge();
		edge_next = (*faces)[i]->getEdge()->getEdgeNext();
		edge_prev = (*faces)[i]->getEdge()->getEdgePrevious();


		// prvy bod
		xBefore = edge->getVertexOrigin()->getX();
		yBefore = edge->getVertexOrigin()->getY();
		zBefore = edge->getVertexOrigin()->getZ();

		distance = qSqrt((xBefore - a) * (xBefore - a) + (yBefore - b) * (yBefore - b) + (zBefore - c) * (zBefore - c)) * camera.getScaleValue();
		if ((distance < camera.getClipNearDistance()) || (distance > camera.getClipFarDistance()))
			continue;

		temp = (a * xBefore + b * yBefore + c * zBefore) / (a * (S.x() - xBefore) + b * (S.y() - yBefore) + c * (S.z() - zBefore));

		xAfter = xBefore - (xBefore - S.x()) * temp;
		yAfter = yBefore - (yBefore - S.y()) * temp;
		zAfter = zBefore - (zBefore - S.z()) * temp;

		//https://stackoverflow.com/questions/23472048/projecting-3d-points-to-2d-plane
		// povodne bol pre x vektor u a pre y vektor v, ale trochu divne to kreslilo
		x2D = v.x() * xAfter + v.y() * yAfter + v.z() * zAfter;
		y2D = u.x() * xAfter + u.y() * yAfter + u.z() * zAfter;

		newPoints[0].setX(x2D * camera.getScaleValue() * objectScale + midPointX + dx);
		newPoints[0].setY(y2D * camera.getScaleValue() * objectScale + midPointY + dy);

		zCoords[0] = zAfter;

		// druhy bod
		xBefore = edge_next->getVertexOrigin()->getX();
		yBefore = edge_next->getVertexOrigin()->getY();
		zBefore = edge_next->getVertexOrigin()->getZ();

		distance = qSqrt((xBefore - a) * (xBefore - a) + (yBefore - b) * (yBefore - b) + (zBefore - c) * (zBefore - c)) * camera.getScaleValue();
		if ((distance < camera.getClipNearDistance()) || (distance > camera.getClipFarDistance()))
			continue;

		temp = (a * xBefore + b * yBefore + c * zBefore) / (a * (S.x() - xBefore) + b * (S.y() - yBefore) + c * (S.z() - zBefore));

		xAfter = xBefore - (xBefore - S.x()) * temp;
		yAfter = yBefore - (yBefore - S.y()) * temp;
		zAfter = zBefore - (zBefore - S.z()) * temp;

		//https://stackoverflow.com/questions/23472048/projecting-3d-points-to-2d-plane
		// povodne bol pre x vektor u a pre y vektor v, ale trochu divne to kreslilo
		x2D = v.x() * xAfter + v.y() * yAfter + v.z() * zAfter;
		y2D = u.x() * xAfter + u.y() * yAfter + u.z() * zAfter;

		newPoints[1].setX(x2D * camera.getScaleValue() * objectScale + midPointX + dx);
		newPoints[1].setY(y2D * camera.getScaleValue() * objectScale + midPointY + dy);

		zCoords[1] = zAfter;

		// treti bod
		xBefore = edge_prev->getVertexOrigin()->getX();
		yBefore = edge_prev->getVertexOrigin()->getY();
		zBefore = edge_prev->getVertexOrigin()->getZ();

		distance = qSqrt((xBefore - a) * (xBefore - a) + (yBefore - b) * (yBefore - b) + (zBefore - c) * (zBefore - c)) * camera.getScaleValue();
		if ((distance < camera.getClipNearDistance()) || (distance > camera.getClipFarDistance()))
			continue;

		temp = (a * xBefore + b * yBefore + c * zBefore) / (a * (S.x() - xBefore) + b * (S.y() - yBefore) + c * (S.z() - zBefore));

		xAfter = xBefore - (xBefore - S.x()) * temp;
		yAfter = yBefore - (yBefore - S.y()) * temp;
		zAfter = zBefore - (zBefore - S.z()) * temp;

		//https://stackoverflow.com/questions/23472048/projecting-3d-points-to-2d-plane
		// povodne bol pre x vektor u a pre y vektor v, ale trochu divne to kreslilo
		x2D = v.x() * xAfter + v.y() * yAfter + v.z() * zAfter;
		y2D = u.x() * xAfter + u.y() * yAfter + u.z() * zAfter;

		newPoints[2].setX(x2D * camera.getScaleValue() * objectScale + midPointX + dx);
		newPoints[2].setY(y2D * camera.getScaleValue() * objectScale + midPointY + dy);

		zCoords[2] = zAfter;

		colors.clear();
		colors.append(edge->getVertexOrigin()->getVertexColor());
		colors.append(edge_next->getVertexOrigin()->getVertexColor());
		colors.append(edge_prev->getVertexOrigin()->getVertexColor());

		//vW->createGeometry(newPoints, QColor("#FFFFFF"), QColor("#000000"), 0);
		//vW->drawPolygon(newPoints, QColor("#FFFFFF"));
		vW->drawPolygonT(newPoints, colors, zCoords, camera.getShadingType());
	}
}

void ImageViewer::calculateColors()
{
	QVector3D L(0.0,0.0,0.0), R(0.0, 0.0, 0.0), V(0.0, 0.0, 0.0), N(0.0, 0.0, 0.0);
	QList<Vertex*>* vertices = octahedron.pointerVertices();
	QList<Face*>* faces = octahedron.pointerFaces();
	Vertex* currentVertex = nullptr;
	double temp = 0.0, red = 0.0, green = 0.0, blue = 0.0;
	int intTemp = 0;
	double h = coeficientsPOM.specular.specularSharpness;
	QColor I = QColor("#000000");
	QColor I_L = lightSource.lightColor; // farba svetla
	QColor I_o = coeficientsPOM.ambient.ambientColor; // farba okolia
	QColor I_s, I_d, I_a; // jednotlive zlozky

	SpecularComponent r_s = coeficientsPOM.specular; // koeficient odrazu
	DiffuseComponent r_d = coeficientsPOM.difuse; // koeficient difuzie
	AmbientComponent r_a = coeficientsPOM.ambient; // ambientny koeficient

	for (int i = 0; i < vertices->size(); i++)
	{
		currentVertex = (*vertices)[i];

		// normala
		N = currentVertex->getVertexNormal();

		// svetelny luc
		L.setX(lightSource.x - currentVertex->getX());
		L.setY(lightSource.y - currentVertex->getY());
		L.setZ(lightSource.z - currentVertex->getZ());

		L.normalize();
		
		// odrazeny luc
		temp = 2.0 * QVector3D::dotProduct(L, N);
		R = temp * N - L;

		R.normalize();
		
		if (camera.getProjectionType() == Projection3D::ParallelProjection)
		{
			V = camera.getVector_n();
		}
		else if (camera.getProjectionType() == Projection3D::PerspectiveProjection)
		{
			V = camera.getVector_n() * camera.getCameraDistance();

			V.setX(V.x() - currentVertex->getX());
			V.setY(V.y() - currentVertex->getY());
			V.setZ(V.z() - currentVertex->getZ());
		}

		V.normalize();

		// zrkadlova zlozka
		if (QVector3D::dotProduct(V, R) <= 0.0)
			I_s = QColor("#000000");
		else if (QVector3D::dotProduct(L, N) <= 0.0)
			I_s = QColor("#000000");
		else
		{
			temp = qPow(QVector3D::dotProduct(V, R), h);
			red = I_L.redF() * r_s.red * temp;
			green = I_L.greenF() * r_s.green * temp;
			blue = I_L.blueF() * r_s.blue * temp;

			I_s.setRedF(red); I_s.setGreenF(green); I_s.setBlueF(blue);
		}

		// diffuzna zlozka
		if (QVector3D::dotProduct(L, N) <= 0.0)
			I_d = QColor("#000000");
		else
		{
			red = I_L.redF() * r_d.red * QVector3D::dotProduct(L, N);
			green = I_L.greenF() * r_d.green * QVector3D::dotProduct(L, N);
			blue = I_L.blueF() * r_d.blue * QVector3D::dotProduct(L, N);

			I_d.setRedF(red); I_d.setGreenF(green); I_d.setBlueF(blue);
		}

		// ambientna zlozka
		red = I_o.redF() * r_a.red;
		green = I_o.greenF() * r_a.green;
		blue = I_o.blueF() * r_a.blue;

		I_a.setRedF(red); I_a.setGreenF(green); I_a.setBlueF(blue);

		// vysledna farba cerveny komponent
		intTemp = I_s.red() + I_d.red() + I_a.red();
		//qDebug() << "I.red=" << intTemp;
		if (intTemp < 0)
		{
			I.setRed(0);
			//qDebug() << "red < 0";
		}
		else if (intTemp > 255)
		{
			I.setRed(255);
			//qDebug() << "red > 255";
		}
		else
		{
			//qDebug() << "setRed here";
			I.setRed(intTemp);
		}

		// vysledna farba zeleny komponent
		intTemp = I_s.green() + I_d.green() + I_a.green();
		//qDebug() << "I.green=" << intTemp;
		if (intTemp < 0)
			I.setGreen(0);
		else if (intTemp > 255)
			I.setGreen(255);
		else
			I.setGreen(intTemp);

		// vysledna farba modry komponent
		intTemp = I_s.blue() + I_d.blue() + I_a.blue();
		//qDebug() << "I.blue=" << intTemp << "\n";
		if (intTemp < 0)
			I.setBlue(0);
		else if (intTemp > 255)
			I.setBlue(255);
		else
			I.setBlue(intTemp);

		currentVertex->setVertexColor(I);

		//qDebug() << QString("vertex[%1] color:").arg(i) + currentVertex->getVertexColor().name();
	}
}

void ImageViewer::update3D()
{
	if (!octahedron.isEmpty())
	{
		octahedron.calculateNormals();

		calculateColors();

		if (camera.getProjectionType() == Projection3D::ParallelProjection)
		{
			parallelProjection();
		}
		else if (camera.getProjectionType() == Projection3D::PerspectiveProjection)
		{
			perspectiveProjection();
		}
	}
}

//ViewerWidget functions
ViewerWidget* ImageViewer::getViewerWidget(int tabId)
{
	QScrollArea* s = static_cast<QScrollArea*>(ui->tabWidget->widget(tabId));
	if (s) {
		ViewerWidget* vW = static_cast<ViewerWidget*>(s->widget());
		return vW;
	}
	return nullptr;
}
ViewerWidget* ImageViewer::getCurrentViewerWidget()
{
	return getViewerWidget(ui->tabWidget->currentIndex());
}

// Event filters
bool ImageViewer::eventFilter(QObject* obj, QEvent* event)
{
	if (obj->objectName() == "ViewerWidget") {
		return ViewerWidgetEventFilter(obj, event);
	}
	return false;
}

//ViewerWidget Events
bool ImageViewer::ViewerWidgetEventFilter(QObject* obj, QEvent* event)
{
	ViewerWidget* w = static_cast<ViewerWidget*>(obj);

	if (!w) {
		return false;
	}

	if (event->type() == QEvent::MouseButtonPress) {
		ViewerWidgetMouseButtonPress(w, event);
	}
	else if (event->type() == QEvent::MouseButtonRelease) {
		ViewerWidgetMouseButtonRelease(w, event);
	}
	else if (event->type() == QEvent::MouseMove) {
		ViewerWidgetMouseMove(w, event);
	}
	else if (event->type() == QEvent::Leave) {
		ViewerWidgetLeave(w, event);
	}
	else if (event->type() == QEvent::Enter) {
		ViewerWidgetEnter(w, event);
	}
	else if (event->type() == QEvent::Wheel) {
		ViewerWidgetWheel(w, event);
	}

	return QObject::eventFilter(obj, event);
}
void ImageViewer::ViewerWidgetMouseButtonPress(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);
	if (e->button() == Qt::LeftButton) {
	}
}
void ImageViewer::ViewerWidgetMouseButtonRelease(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);
}
void ImageViewer::ViewerWidgetMouseMove(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);
}
void ImageViewer::ViewerWidgetLeave(ViewerWidget* w, QEvent* event)
{
}
void ImageViewer::ViewerWidgetEnter(ViewerWidget* w, QEvent* event)
{
}
void ImageViewer::ViewerWidgetWheel(ViewerWidget* w, QEvent* event)
{
	QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
}

//ImageViewer Events
void ImageViewer::closeEvent(QCloseEvent* event)
{
	if (QMessageBox::Yes == QMessageBox::question(this, "Close Confirmation", "Are you sure you want to exit?", QMessageBox::Yes | QMessageBox::No))
	{
		if (!octahedron.isEmpty())
			octahedron.clear();

		event->accept();
	}
	else {
		event->ignore();
	}
}

//Image functions
void ImageViewer::openNewTabForImg(ViewerWidget* vW)
{
	QScrollArea* scrollArea = new QScrollArea;
	scrollArea->setWidget(vW);

	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	vW->setObjectName("ViewerWidget");
	vW->installEventFilter(this);

	QString name = vW->getName();

	ui->tabWidget->addTab(scrollArea, name);
}
bool ImageViewer::openImage(QString filename)
{
	QFileInfo fi(filename);

	QString name = fi.baseName();
	openNewTabForImg(new ViewerWidget(name, QSize(0, 0)));
	ui->tabWidget->setCurrentIndex(ui->tabWidget->count() - 1);

	ViewerWidget* w = getCurrentViewerWidget();

	QImage loadedImg(filename);
	return w->setImage(loadedImg);
}
bool ImageViewer::saveImage(QString filename)
{
	QFileInfo fi(filename);
	QString extension = fi.completeSuffix();
	ViewerWidget* w = getCurrentViewerWidget();

	QImage* img = w->getImage();
	return img->save(filename, extension.toStdString().c_str());
}
void ImageViewer::clearImage()
{
	ViewerWidget* w = getCurrentViewerWidget();
	w->clear();
}
void ImageViewer::setBackgroundColor(QColor color)
{
	ViewerWidget* w = getCurrentViewerWidget();
	w->clear(color);
}

//Slots

//Tabs slots
void ImageViewer::on_tabWidget_tabCloseRequested(int tabId)
{
	ViewerWidget* vW = getViewerWidget(tabId);
	delete vW; //vW->~ViewerWidget();
	ui->tabWidget->removeTab(tabId);
}
void ImageViewer::on_actionRename_triggered()
{
	if (!isImgOpened()) {
		msgBox.setText("No image is opened.");
		msgBox.setIcon(QMessageBox::Information);
		msgBox.exec();
		return;
	}
	ViewerWidget* w = getCurrentViewerWidget();
	bool ok;
	QString text = QInputDialog::getText(this, QString("Rename image"), tr("Image name:"), QLineEdit::Normal, w->getName(), &ok);
	if (ok && !text.trimmed().isEmpty())
	{
		w->setName(text);
		ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), text);
	}
}

//Image slots
void ImageViewer::on_actionNew_triggered()
{
	newImgDialog = new NewImageDialog(this);
	connect(newImgDialog, SIGNAL(accepted()), this, SLOT(newImageAccepted()));
	newImgDialog->exec();
}
void ImageViewer::newImageAccepted()
{
	NewImageDialog* newImgDialog = static_cast<NewImageDialog*>(sender());

	int width = newImgDialog->getWidth();
	int height = newImgDialog->getHeight();
	QString name = newImgDialog->getName();
	openNewTabForImg(new ViewerWidget(name, QSize(width, height)));
	ui->tabWidget->setCurrentIndex(ui->tabWidget->count() - 1);
}
void ImageViewer::on_actionOpen_triggered()
{
	QString folder = settings.value("folder_img_load_path", "").toString();

	QString fileFilter = "Image data (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm .*xbm .* xpm);;All files (*)";
	QString fileName = QFileDialog::getOpenFileName(this, "Load image", folder, fileFilter);
	if (fileName.isEmpty()) { return; }

	QFileInfo fi(fileName);
	settings.setValue("folder_img_load_path", fi.absoluteDir().absolutePath());

	if (!openImage(fileName)) {
		msgBox.setText("Unable to open image.");
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.exec();
	}
}
void ImageViewer::on_actionSave_as_triggered()
{
	if (!isImgOpened()) {
		msgBox.setText("No image to save.");
		msgBox.setIcon(QMessageBox::Information);
		msgBox.exec();
		return;
	}
	QString folder = settings.value("folder_img_save_path", "").toString();

	ViewerWidget* w = getCurrentViewerWidget();

	QString fileFilter = "Image data (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm .*xbm .* xpm);;All files (*)";
	QString fileName = QFileDialog::getSaveFileName(this, "Save image", folder + "/" + w->getName(), fileFilter);
	if (fileName.isEmpty()) { return; }

	QFileInfo fi(fileName);
	settings.setValue("folder_img_save_path", fi.absoluteDir().absolutePath());

	if (!saveImage(fileName)) {
		msgBox.setText("Unable to save image.");
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.exec();
	}
	else {
		msgBox.setText(QString("File %1 saved.").arg(fileName));
		msgBox.setIcon(QMessageBox::Information);
		msgBox.exec();
	}
}
void ImageViewer::on_actionClear_triggered()
{
	if (!isImgOpened()) {
		msgBox.setText("No image is opened.");
		msgBox.setIcon(QMessageBox::Information);
		msgBox.exec();
		return;
	}
	clearImage();
}
void ImageViewer::on_actionSet_background_color_triggered()
{
	QColor backgroundColor = QColorDialog::getColor(Qt::white, this, "Select color of background");
	if (backgroundColor.isValid()) {
		setBackgroundColor(backgroundColor);
	}
}

void ImageViewer::on_pushButton_Create_clicked()
{
	ui->pushButton_Clear->setEnabled(true);
	ui->pushButton_Create->setEnabled(false);
	ui->pushButton_Export->setEnabled(true);
	ui->pushButton_Import->setEnabled(false);

	//qDebug() << "Before\nVertices size:" << octahedron.Vertices().size();
	//qDebug() << "Edges size:" << octahedron.Edges().size();
	//qDebug() << "Faces size:" << octahedron.Faces().size();

	QList<Vertex*>* vertices = octahedron.pointerVertices();
	QList<H_edge*>* edges = octahedron.pointerEdges();
	QList<Face*>* faces = octahedron.pointerFaces();

	vertices->reserve(6);
	edges->reserve(24); // dvojnasobny pocet hran, lebo H_edge
	faces->reserve(8);

	// naplnenie pociatocnych vrcholov
	for (int i = 0; i < 6; i++)
	{
		vertices->append(new Vertex());
		(*vertices)[i]->setIndex(i); // dereferencia na QList<Vertex*> zo smerniku na QList<Vertex*>
	}

	// naplnenie pociatocnych hran
	for (int i = 0; i < 24; i++)
		(*edges).append(new H_edge());

	// naplnenie pociatocnych stien
	for (int i = 0; i < 8; i++)
		(*faces).append(new Face());

	// k tomuto je .blend subor, z ktoreho sa vsetko vypisovalo

	// VRCHOLY
	(*vertices)[0]->setCoordinates(1.0, 0.0, 0.0); // bod [1,0,0]
	(*vertices)[0]->setEdge((*edges)[0]);
	
	(*vertices)[1]->setCoordinates(0.0, 1.0, 0.0); // bod [0,1,0]
	(*vertices)[1]->setEdge((*edges)[3]);
	
	(*vertices)[2]->setCoordinates(-1.0, 0.0, 0.0); // bod [-1,0,0]
	(*vertices)[2]->setEdge((*edges)[7]);

	(*vertices)[3]->setCoordinates(0.0, -1.0, 0.0); // bod [0,-1,0]
	(*vertices)[3]->setEdge((*edges)[11]);

	(*vertices)[4]->setCoordinates(0.0, 0.0, 1.0); // bod [0,0,1]
	(*vertices)[4]->setEdge((*edges)[2]);

	(*vertices)[5]->setCoordinates(0.0, 0.0, -1.0); // bod [0,0,-1]
	(*vertices)[5]->setEdge((*edges)[17]);

	qDebug() << "points done";

	// HRANY (Vertex* vert_origin, Face* face, H_edge* edge_prev, H_edge* edge_next, H_edge* pair)
	// zapis zmeneny zo zapisu podla prislusnosti ku ploske na zapis po paroch
	(*edges)[0]->setAll((*vertices)[0], (*faces)[0], (*edges)[4], (*edges)[2], (*edges)[1]); // 0, 0, 2, 4, 1
	(*edges)[1]->setAll((*vertices)[4], (*faces)[3], (*edges)[11], (*edges)[14], (*edges)[0]); // 4, 3, 11, 14, 0

	(*edges)[2]->setAll((*vertices)[4], (*faces)[0], (*edges)[0], (*edges)[4], (*edges)[3]); // 4, 0, 0, 4, 3
	(*edges)[3]->setAll((*vertices)[1], (*faces)[1], (*edges)[8], (*edges)[6], (*edges)[2]); // 1, 1, 8, 6, 2

	(*edges)[4]->setAll((*vertices)[1], (*faces)[0], (*edges)[2], (*edges)[0], (*edges)[5]); // 1, 0, 2, 0, 5
	(*edges)[5]->setAll((*vertices)[0], (*faces)[4], (*edges)[17], (*edges)[20], (*edges)[4]); // 0, 4, 17, 20, 4

	(*edges)[6]->setAll((*vertices)[4], (*faces)[1], (*edges)[3], (*edges)[8], (*edges)[7]); // 4, 1, 3, 8, 7
	(*edges)[7]->setAll((*vertices)[2], (*faces)[2], (*edges)[12], (*edges)[10], (*edges)[6]); // 2, 2, 12, 10, 6

	(*edges)[8]->setAll((*vertices)[2], (*faces)[1], (*edges)[6], (*edges)[3], (*edges)[9]); // 2, 1, 6, 3, 9
	(*edges)[9]->setAll((*vertices)[1], (*faces)[5], (*edges)[21], (*edges)[22], (*edges)[8]); // 1, 5, 21, 22, 8

	(*edges)[10]->setAll((*vertices)[4], (*faces)[2], (*edges)[7], (*edges)[12], (*edges)[11]); // 4, 2, 7, 12, 11
	(*edges)[11]->setAll((*vertices)[3], (*faces)[3], (*edges)[14], (*edges)[1], (*edges)[10]); // 3, 3, 14, 1, 10

	(*edges)[12]->setAll((*vertices)[3], (*faces)[2], (*edges)[10], (*edges)[7], (*edges)[13]); // 3, 2, 10, 7, 13 
	(*edges)[13]->setAll((*vertices)[2], (*faces)[6], (*edges)[23], (*edges)[19], (*edges)[12]); // 2, 6, 23, 19, 12

	(*edges)[14]->setAll((*vertices)[0], (*faces)[3], (*edges)[1], (*edges)[11], (*edges)[15]); // 0, 3, 1, 11, 15
	(*edges)[15]->setAll((*vertices)[3], (*faces)[7], (*edges)[18], (*edges)[16], (*edges)[14]); // 3, 7, 18, 16, 14

	(*edges)[16]->setAll((*vertices)[0], (*faces)[7], (*edges)[15], (*edges)[18], (*edges)[17]); // 0, 7, 15, 18, 17
	(*edges)[17]->setAll((*vertices)[5], (*faces)[4], (*edges)[20], (*edges)[5], (*edges)[16]); // 5, 4, 20, 5, 16

	(*edges)[18]->setAll((*vertices)[5], (*faces)[7], (*edges)[16], (*edges)[15], (*edges)[19]); // 5, 7, 16, 15, 19
	(*edges)[19]->setAll((*vertices)[3], (*faces)[6], (*edges)[13], (*edges)[23], (*edges)[18]); // 3, 6, 13, 23, 18

	(*edges)[20]->setAll((*vertices)[1], (*faces)[4], (*edges)[5], (*edges)[17], (*edges)[21]); // 1, 4, 5, 17, 21
	(*edges)[21]->setAll((*vertices)[5], (*faces)[5], (*edges)[22], (*edges)[9], (*edges)[20]); // 5, 5, 22, 9, 20

	(*edges)[22]->setAll((*vertices)[2], (*faces)[5], (*edges)[9], (*edges)[21], (*edges)[23]); // 2, 5, 9, 21, 23
	(*edges)[23]->setAll((*vertices)[5], (*faces)[6], (*edges)[19], (*edges)[13], (*edges)[22]); // 5, 6, 19, 13, 22

	qDebug() << "edges done";
	/*for (int i = 0; i < edges->size(); i++)
	{
		if ((*edges)[i]->hasPair())
		{
			qDebug() << QString("edges[%1]:").arg(i) << (*edges)[i]->edgeVerticesInfo();
			qDebug() << QString("edges[%1] pair:").arg(i) << (*edges)[i]->getEdgePair()->edgeVerticesInfo() << "\n";
		}
	}*/

	// STENY
	(*faces)[0]->setEdge((*edges)[0]);
	(*faces)[1]->setEdge((*edges)[3]);
	(*faces)[2]->setEdge((*edges)[7]);
	(*faces)[3]->setEdge((*edges)[11]);
	(*faces)[4]->setEdge((*edges)[20]);
	(*faces)[5]->setEdge((*edges)[22]);
	(*faces)[6]->setEdge((*edges)[19]);
	(*faces)[7]->setEdge((*edges)[16]);

	qDebug() << "faces done\n\nall done";

	subdivideOctahedron(ui->spinBox_Divisions->value());
	
	infoMessage(QString("Number of divisions: %1 done").arg(ui->spinBox_Divisions->value()));
	ui->groupBox_Divisions->setEnabled(false);

	update3D();

	// asi kontrolne vypisovanie informacii
	/*
	for (int i = 0; i < edges->size(); i++)
	{
		if ((*edges)[i]->getEdgePair() != nullptr)
			qDebug() << QString("edges[%1]:").arg(i) << (*edges)[i]->edgeVerticesInfo();
		else
			qDebug() << QString("edges[%1] has no edge pair").arg(i);
	}

	
	for (int i = 0; i < vertices->size(); i++)
		qDebug() << QString("vertices[%1]:").arg(i) + (*vertices)[i]->vertexInfo();

	for (int i = 0; i < edges->size(); i++)
	{
		//if ((*edges)[i]->hasPair())
			qDebug() << QString("edges[%1]:").arg(i) << (*edges)[i]->edgeVerticesInfo();
	}

	for (int i = 0; i < faces->size(); i++)
		qDebug() << QString("faces[%1]:").arg(i) << (*faces)[i]->faceVerticesInfo();
	*/
}
void ImageViewer::on_pushButton_Clear_clicked()
{
	ui->pushButton_Clear->setEnabled(false);
	ui->pushButton_Create->setEnabled(true);
	ui->groupBox_Divisions->setEnabled(true);
	ui->pushButton_Export->setEnabled(false);
	ui->pushButton_Import->setEnabled(true);

	if (!octahedron.isEmpty())
		octahedron.clear();

	getCurrentViewerWidget()->clear();
}
void ImageViewer::on_pushButton_Export_clicked()
{
	if (!exportOctahedron())
		warningMessage("Export Error");
	else
		infoMessage("Export Done");
}
void ImageViewer::on_pushButton_Import_clicked()
{
	if (!importOctahedron())
		warningMessage("Import Error");
	else
	{
		infoMessage("Import Done");
		ui->pushButton_Import->setEnabled(false);
		ui->pushButton_Create->setEnabled(false);
		ui->pushButton_Export->setEnabled(true);
		ui->pushButton_Clear->setEnabled(true);

		update3D();
	}
}

void ImageViewer::on_horizontalSlider_Zenit_valueChanged(int angle)
{
	camera.setZenit((angle * M_PI) / 180.0);
	update3D();
}
void ImageViewer::on_horizontalSlider_Azimut_valueChanged(int angle)
{
	camera.setAzimut((angle * M_PI) / 180.0);
	update3D();
}

void ImageViewer::on_doubleSpinBox_CameraDistance_valueChanged(double value)
{
	camera.setCameraDistance(value);
	update3D();
}

void ImageViewer::on_doubleSpinBox_ObjectScale_valueChanged(double value)
{
	update3D();
}

void ImageViewer::on_horizontalSlider_dx_valueChanged(int dx)
{
	update3D();
}

void ImageViewer::on_horizontalSlider_dy_valueChanged(int dy)
{
	update3D();
}

void ImageViewer::on_radioButton_Parallel_clicked()
{
	camera.setProjectionType(Projection3D::ParallelProjection);
	update3D();
}

void ImageViewer::on_radioButton_Perspective_clicked()
{
	camera.setProjectionType(Projection3D::PerspectiveProjection);
	update3D();
}

void ImageViewer::on_doubleSpinBox_ClipNear_valueChanged(double value)
{
	camera.setClipNearDistance(value);
	update3D();
}

void ImageViewer::on_doubleSpinBox_ClipFar_valueChanged(double value)
{
	camera.setClipFarDistance(value);
	update3D();
}

void ImageViewer::on_horizontalSlider_LightX_valueChanged(int value)
{
	lightSource.x = (double)value;
	update3D();
}
void ImageViewer::on_horizontalSlider_LightY_valueChanged(int value)
{
	lightSource.y = (double)value;
	update3D();
}
void ImageViewer::on_horizontalSlider_LightZ_valueChanged(int value)
{
	lightSource.z = (double)value;
	update3D();
}
void ImageViewer::on_pushButton_LightColor_clicked()
{
	QColor chosenColor = QColorDialog::getColor(lightSource.lightColor.name(), this, "Select light source color");

	if (chosenColor.isValid())
	{
		lightSource.lightColor = chosenColor;
		ui->pushButton_LightColor->setStyleSheet(QString("background-color:%1").arg(chosenColor.name()));
		update3D();
	}
	else
		warningMessage("Invalid color");
}

void ImageViewer::on_doubleSpinBox_DiffuseRed_valueChanged(double value)
{
	coeficientsPOM.difuse.red = value;
	update3D();
}
void ImageViewer::on_doubleSpinBox_DiffuseGreen_valueChanged(double value)
{
	coeficientsPOM.difuse.green = value;
	update3D();
}
void ImageViewer::on_doubleSpinBox_DiffuseBlue_valueChanged(double value)
{
	coeficientsPOM.difuse.blue = value;
	update3D();
}

void ImageViewer::on_doubleSpinBox_SpecularRed_valueChanged(double value)
{
	coeficientsPOM.specular.red = value;
	update3D();
}
void ImageViewer::on_doubleSpinBox_SpecularGreen_valueChanged(double value)
{
	coeficientsPOM.specular.green = value;
	update3D();
}
void ImageViewer::on_doubleSpinBox_SpecularBlue_valueChanged(double value)
{
	coeficientsPOM.specular.blue = value;
	update3D();
}
void ImageViewer::on_doubleSpinBox_SpecularSharpness_valueChanged(double value)
{
	coeficientsPOM.specular.specularSharpness = value;
	update3D();
}

void ImageViewer::on_doubleSpinBox_AmbientRed_valueChanged(double value)
{
	coeficientsPOM.ambient.red = value;
	update3D();
}
void ImageViewer::on_doubleSpinBox_AmbientGreen_valueChanged(double value)
{
	coeficientsPOM.ambient.green = value;
	update3D();
}
void ImageViewer::on_doubleSpinBox_AmbientBlue_valueChanged(double value)
{
	coeficientsPOM.ambient.blue = value;
	update3D();
}
void ImageViewer::on_pushButton_AmbientColor_clicked()
{
	QColor chosenColor = QColorDialog::getColor(coeficientsPOM.ambient.ambientColor.name(), this, "Select ambient color");

	if (chosenColor.isValid())
	{
		coeficientsPOM.ambient.ambientColor = chosenColor;
		ui->pushButton_AmbientColor->setStyleSheet(QString("background-color:%1").arg(chosenColor.name()));
		update3D();
	}
	else
		warningMessage("Invalid color");
}

void ImageViewer::on_radioButton_ConstantShading_clicked()
{
	camera.setShadingType(Projection3D::ConstantShading);
	update3D();
}

void ImageViewer::on_radioButton_GouraudShading_clicked()
{
	camera.setShadingType(Projection3D::GouraudShading);
	update3D();
}

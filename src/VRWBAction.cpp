#include "VRWBAction.h"

#include "WorldBuilderScene.h"
#include "WorldBuilderSystem.h"

void VRSculptSet::IdleDrawCallback(void)
{
	if(m_pDrawAction != 0)
	{
		if(m_pDrawAction->GetType() == VRAction::WAND)
		{
			static_cast<VRWandAction*>(m_pDrawAction)->DrawCallback();
		}
	}
}

void VRChangeBrush::ButtonUp(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	wbScene->changeAddMode();
}

void VRChangeSpecificTool::ButtonUp(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	wbScene->setTool(m_tool);
}

void VRChangeTool::ButtonUp(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	wbScene->changeTool();
}

void VRChangeTSR::ButtonUp(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	wbScene->changeTSR();
}

void VRDelete::ButtonUp(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	if(wbScene->numSelected() > 0)
	{
		Ogre::MovableObject *pObj = wbScene->getSelected(0);
		if(pObj)
		{
			//remove object from selection..
			wbScene->clearSelection();
			Ogre::SceneNode *pParent = pObj->getParentSceneNode();
			pParent->detachObject(pObj);
			wbScene->getScene()->destroyEntity(pObj->getName());
			wbScene->getScene()->destroySceneNode(pParent->getName());
            // Play a sound!
            if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
                wbScene->playSound("delete");
            }
		}
	}
}

void VRDuplicate::ButtonUp(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	if(wbScene->numSelected() > 0)
	{
		Ogre::MovableObject *pObj = wbScene->getSelected(0);
		if(pObj)
		{
			Ogre::MeshPtr p = static_cast<Ogre::Entity*>(pObj)->getMesh();
			static int dCount = 0;
			Ogre::String sName = pObj->getName();
			char newName[256];
			memset(newName, 0, 256);
			//todo - fix up this naming..
			sprintf(newName, "%s%d", sName.c_str(), dCount);
			dCount++;
			
			const Ogre::Vector3 & vPos = pObj->getParentSceneNode()->getPosition();
			float fXMove = pObj->getBoundingBox().getSize().x;
			Ogre::SceneNode* headNode = wbScene->getScene()->getRootSceneNode()->createChildSceneNode();
			headNode->setPosition(vPos.x + fXMove, vPos.y, vPos.z);
			const Ogre::Vector3 &vScale = pObj->getParentSceneNode()->getScale();
			const Ogre::Quaternion &qRot = pObj->getParentSceneNode()->getOrientation();
			headNode->setScale(vScale.x, vScale.y, vScale.z);
			headNode->setOrientation(qRot.w, qRot.x, qRot.y, qRot.z);
			headNode->attachObject(wbScene->getScene()->createEntity(newName, p));
            // Play a sound!
            if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
                wbScene->playSound("duplicate");
            }
		}
	}
}

void VRErase::ButtonDown(void)
{
	WandMove();
}

void VRErase::WandMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	
	jvec3 pos = wbScene->getNormalizedWandPos();

	jvec3 vWandDir;
	//float toDeg = wbScene->getWiiFitRotation() * PI/180;
	//mat3 m(cosf(toDeg), 0.f, -sinf(toDeg), 0.f, 1.f, 0.f, sinf(toDeg), 0.f, cosf(toDeg));
	wbScene->getWandDirWorldSpace(vWandDir, true, wbScene->getWiiFitRotation());
	//vWandDir = m.transpose().inv()  * vWandDir;
	jvec3 wp = (wbScene->getBeamLength() * vWandDir);
	jvec3 wpos = pos+(wp /(wbScene->getCubeScale()*2.f));
	m_beamPoint = pos + (wbScene->getBeamLength() * vWandDir);
	wbScene->getSystem()->erase(pos.x, pos.y, pos.z, wpos.x, wpos.y, wpos.z, wbScene->getBeamRadius(), wbScene->getCubeScale());
}

void VRErase::ButtonUp(void)
{

}

void VRErase::JoystickMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	float beamRadius = wbScene->getBeamRadius();
	if(beamRadius > 0.01f)
	{
		beamRadius += 0.001 * fionaConf.currentJoystick.x;
		wbScene->setBeamRadius(beamRadius);
	}

	float beamLength = wbScene->getBeamLength();
	if(beamLength > 0.01f)
	{
		beamLength += 0.001 * fionaConf.currentJoystick.z;
		wbScene->setBeamLength(beamLength);
	}
}

void VRErase::DrawCallback(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	
	float beamLength = wbScene->getBeamLength();
	float beamRadius = wbScene->getBeamRadius();

	jvec3 pos = wbScene->getWandPos();

	jvec3 vWandDir;
	//float toDeg = wbScene->getWiiFitRotation() * PI/180;
    //mat3 m(cosf(toDeg), 0.f, -sinf(toDeg), 0.f, 1.f, 0.f, sinf(toDeg), 0.f, cosf(toDeg));
    wbScene->getWandDirWorldSpace(vWandDir, true, wbScene->getWiiFitRotation());
    //vWandDir = m  * vWandDir;
	vWandDir = vWandDir.normalize();
	jvec3 vWandUp;
	wbScene->getWandDirWorldSpace(vWandUp, false, wbScene->getWiiFitRotation());
	//vWandUp = m * vWandUp;
	vWandUp = vWandUp.normalize();
	jvec3 vWandLeft = vWandDir * vWandUp;
	vWandLeft = vWandLeft.normalize();

	m_beamPoint = pos + (beamLength * vWandDir);

	jvec3 dir = m_beamPoint - pos;

	if (dir.z == 0.0) {
		dir.z = 0.0001;
	}

	float len = dir.len();

	// draw the cylinder
	glPushMatrix();
	glDisable(GL_LIGHTING);
	glColor3f(1.0f, 1.0f, 1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//drawcylinder manually..
	glBegin(GL_POLYGON);
	int numSides = 12;
	for(int i = 0; i <= 360; i+=360/numSides)
	{
		float a = i * PI / 180;
		jvec3 vert = pos + (vWandLeft * cos(a) * beamRadius * 0.1f);
		vert = vert + (vWandUp * sin(a) * beamRadius * 0.1f);
		glVertex3f(vert.x, vert.y, vert.z);
	}
	glEnd();

	glBegin(GL_POLYGON);
	for(int i = 0; i <= 360; i+=360/numSides)
	{
		float a = i * PI / 180;
		jvec3 vert = m_beamPoint + (vWandLeft * cos(a) * beamRadius * 0.1f);
		vert = vert + (vWandUp * sin(a) * beamRadius * 0.1f);
		glVertex3f(vert.x, vert.y, vert.z);
	}
	glEnd();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	glBegin(GL_LINES);
	for(int i = 0; i <= 360; i+=360/numSides)
	{
		float a = i * PI / 180;
		jvec3 vert = m_beamPoint + (vWandLeft * cos(a) * beamRadius * 0.1f);
		vert = vert + (vWandUp * sin(a) * beamRadius * 0.1f);
		glVertex3f(vert.x, vert.y, vert.z);
		jvec3 vert2 = pos + (vWandLeft * cos(a) * beamRadius * 0.1f);
		vert2 = vert2 + (vWandUp * sin(a) * beamRadius * 0.1f);
		glVertex3f(vert2.x, vert2.y, vert2.z);
	}
	glEnd();

	glEnable(GL_LIGHTING);
	glPopMatrix();
}

//todo - maybe throw this into the scene class or some helper utility namespace..
void GetMeshInformation(const Ogre::Mesh* const mesh,
                        size_t &vertex_count,
                        Ogre::Vector3* &vertices,
						Ogre::Vector3* &norms,
						Ogre::Vector3* &colors,
                        size_t &index_count,
                        unsigned long* &indices,
                        const Ogre::Vector3 &position,
                        const Ogre::Quaternion &orient,
                        const Ogre::Vector3 &scale)
{
    bool added_shared = false;
    size_t current_offset = 0;
    size_t shared_offset = 0;
    size_t next_offset = 0;
    size_t index_offset = 0;
 
    vertex_count = index_count = 0;
 
    // Calculate how many vertices and indices we're going to need
    for ( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh(i);
        // We only need to add the shared vertices once
        if(submesh->useSharedVertices)
        {
            if( !added_shared )
            {
                vertex_count += mesh->sharedVertexData->vertexCount;
                added_shared = true;
            }
        }
        else
        {
            vertex_count += submesh->vertexData->vertexCount;
        }
        // Add the indices
        index_count += submesh->indexData->indexCount;
    }
 
    // Allocate space for the vertices and indices
    vertices = new Ogre::Vector3[vertex_count];
	norms = new Ogre::Vector3[vertex_count];
	colors = new Ogre::Vector3[vertex_count];
    indices = new unsigned long[index_count];
 
    added_shared = false;
	Ogre::Matrix3 mat;
	orient.ToRotationMatrix(mat);
	Ogre::Matrix3 invTrans = mat.Inverse();
	invTrans = invTrans.Transpose();

    // Run through the submeshes again, adding the data into the arrays
    for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh(i);
 
        Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;
 
        if ((!submesh->useSharedVertices) || (submesh->useSharedVertices && !added_shared))
        {
            if(submesh->useSharedVertices)
            {
                added_shared = true;
                shared_offset = current_offset;
            }
 
            const Ogre::VertexElement* posElem =
                vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
 
            Ogre::HardwareVertexBufferSharedPtr vbuf =
                vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
 
            unsigned char* vertex =
                static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
 
            // There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
            //  as second argument. So make it float, to avoid trouble when Ogre::Real will
            //  be comiled/typedefed as double:
            //Ogre::Real* pReal;
            float* pReal;
 
            for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
            {
                posElem->baseVertexPointerToElement(vertex, &pReal);
                Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);
                vertices[current_offset + j] = (orient * (pt * scale)) + position;
            }
 
            vbuf->unlock();

            posElem =
                vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_NORMAL);
 
            vbuf =
                vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
 
            vertex =
                static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

            for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
            {
                posElem->baseVertexPointerToElement(vertex, &pReal);
                Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);
                norms[current_offset + j] = (invTrans * pt);
            }
 
            vbuf->unlock();

            posElem =
                vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_DIFFUSE);
 
            vbuf =
                vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
 
            vertex =
                static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
 
			Ogre::RGBA *pColor;
			Ogre::ColourValue c;
			
            for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
            {
                posElem->baseVertexPointerToElement(vertex, &pColor);
				c.setAsABGR(*pColor);
                Ogre::Vector3 pt(c.r, c.g, c.b);
                colors[current_offset + j] = pt;
            }
 
            vbuf->unlock();
			
            next_offset += vertex_data->vertexCount;
        }
 
        Ogre::IndexData* index_data = submesh->indexData;
        size_t numTris = index_data->indexCount / 3;
        Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;
 
        bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);
 
        unsigned long* pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
        unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);
 
        size_t offset = (submesh->useSharedVertices)? shared_offset : current_offset;
 
        if ( use32bitindexes )
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                indices[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
            }
        }
        else
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                indices[index_offset++] = static_cast<unsigned long>(pShort[k]) +
                                          static_cast<unsigned long>(offset);
            }
        }
 
        ibuf->unlock();
        current_offset = next_offset;
    }
}

void WritePLY(const char *fname, Ogre::Vector3 *vertices, Ogre::Vector3 *norms, Ogre::Vector3 *colors, size_t vertex_count, unsigned long *indices, size_t index_count, bool binary)
{
	 FILE *file = 0;
	 if(binary)
	 {
		 file = fopen(fname, "wb");
	 }
	 else
	 {
		file = fopen(fname, "w");
	 }

	 //I am assuming the indices are in order
	 int nfaces = index_count / 3;
 
	 fprintf(file, "ply\n");//
	 if (binary)
		 fprintf(file, "format binary_little_endian 1.0\n");
	 else
		 fprintf(file, "format ascii 1.0\n");// { ascii/binary, format version number }

	 fprintf(file, "element vertex %d\n", vertex_count);// 
	 fprintf(file, "property float x\n");// { vertex contains float "x" coordinate }
	 fprintf(file, "property float y\n");// { y coordinate is also a vertex property }
	 fprintf(file, "property float z\n");// { z coordinate, too }
	 //normals
	 fprintf(file, "property float nx\n");// 
	 fprintf(file, "property float ny\n");// 
	 fprintf(file, "property float nz\n");// 
	 //colors
	 fprintf(file, "property uchar red\n");// 
	 fprintf(file, "property uchar green\n");// 
	 fprintf(file, "property uchar blue\n");// 
	 fprintf(file, "property uchar alpha\n");
 
	 fprintf(file, "element face %d\n", nfaces);// 
	 //note, I don't know what this next line does
	 fprintf(file, "property list uchar int vertex_indices\n");//
	 //{ L"vertex_indices" is a list of ints }
 
	 fprintf(file, "end_header\n");// { delimits the end of the header }
 
	 for (unsigned int i=0; i < vertex_count; i++)
	 {
		 norms[i].normalise();
		 if (binary)
		 {
			 //just to be safe I am doing these one by one
			 fwrite(&vertices[i].x,sizeof(float), 1, file);
			 fwrite(&vertices[i].y,sizeof(float), 1, file);
			 fwrite(&vertices[i].z,sizeof(float), 1, file);
			 fwrite(&norms[i].x,sizeof(float), 1, file);
			 fwrite(&norms[i].y,sizeof(float), 1, file);
			 fwrite(&norms[i].z,sizeof(float), 1, file);

			 unsigned char ucolors[4];
			 ucolors[0] = (unsigned char)(colors[i].x * 255);
			 ucolors[1] = (unsigned char)(colors[i].y * 255);
			 ucolors[2] = (unsigned char)(colors[i].z * 255);
			 ucolors[3] = 255;
			 fwrite(&ucolors[0],sizeof(unsigned char), 4, file);
			 
		 }
		 else
			fprintf(file, "%f %f %f %f %f %f %d %d %d %d\n", vertices[i].x, vertices[i].y, vertices[i].z, norms[i].x, norms[i].y, norms[i].z, (unsigned char)(colors[i].x * 255), (unsigned char)(colors[i].y * 255), (unsigned char)(colors[i].z * 255), 255);
	 }
	 for (unsigned int i=0; i < index_count; i+=3)
	 {
		  if (binary)
		  {
			   unsigned char unum = 3;
			   fwrite(&unum,sizeof(unsigned char), 1, file);
			   fwrite(&indices[i],sizeof(int), 1, file);
			   fwrite(&indices[i+1],sizeof(int), 1, file);
			   fwrite(&indices[i+2],sizeof(int), 1, file);
		  }
		  else
			 fprintf(file, "3 %d %d %d\n", indices[i], indices[i+1], indices[i+2]);
	 }
 
	 //i think that's it
	 fclose(file);
}

void VRExportPly::ButtonUp(void)
{
	if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) 
	{
		WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
		Ogre::SceneManager::MovableObjectIterator iter = wbScene->getScene()->getMovableObjectIterator("Entity");
		while(iter.hasMoreElements())
		{
			Ogre::Entity *e = static_cast<Ogre::Entity*>(iter.getNext());
			if(e->getName().compare("Floor") != 0)
			{
				printf("exporting %s's ply\n", e->getName().c_str());
				const Ogre::Vector3 &vPos = e->getParentSceneNode()->getPosition();
				const Ogre::Vector3 &vScale = e->getParentSceneNode()->getScale();
				const Ogre::Quaternion &qRot = e->getParentSceneNode()->getOrientation();
				const Ogre::MeshPtr &p = e->getMesh();
				size_t vertex_count,index_count;
				Ogre::Vector3* vertices;
				Ogre::Vector3* norms;
				Ogre::Vector3* colors;
				unsigned long* indices;

				GetMeshInformation(p.get(), vertex_count, vertices, norms, colors, index_count, indices, vPos, qRot, vScale);
			
				std::string modelname =  wbScene->getLoggingDirectory();
				modelname += "/";
				modelname += e->getName();
				modelname += ".ply";

				WritePLY(modelname.c_str(), vertices, norms, colors, vertex_count, indices, index_count, true);

				delete[] vertices;
				delete[] norms;
				delete[] colors;
				delete[] indices;
			}
		}
	}
}

void VRJoystickResize::JoystickMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	if(wbScene->isWorldMode())
	{
		if(wbScene->getTSR() == WorldBuilderScene::TRANSLATE)
		{

		}
		else if(wbScene->getTSR() == WorldBuilderScene::SCALE)
		{
			Ogre::MovableObject *pObj = wbScene->getSelected(0);
			if(pObj)
			{
				Ogre::Vector3 vScale = pObj->getParentSceneNode()->getScale();
				//printf("curr joystick: %f, %f\n", fionaConf.currentJoystick.x, fionaConf.currentJoystick.z);
				if(fionaConf.currentJoystick.x > 0.f)
				{
					vScale.x += 0.005f;
					vScale.y += 0.005f;
					vScale.z += 0.005f;
					pObj->getParentSceneNode()->setScale(vScale);
				}
				else if(fionaConf.currentJoystick.x < 0.f)
				{
					vScale.x -= 0.005f;
					vScale.y -= 0.005f;
					vScale.z -= 0.005f;
					pObj->getParentSceneNode()->setScale(vScale);
				}
			}
		}
		else if(wbScene->getTSR() == WorldBuilderScene::ROTATE)
		{
			if(wbScene->numSelected() > 0)
			{
				Ogre::MovableObject *pObj = wbScene->getSelected(0);
				if(pObj)
				{
					const Ogre::Quaternion &qRot = pObj->getParentSceneNode()->getOrientation();
					Ogre::Quaternion qMult = Ogre::Quaternion::IDENTITY;
					if(fionaConf.currentJoystick.x < 0.f)
					{
						qMult.FromAngleAxis(Ogre::Radian::Radian(-0.05f), Ogre::Vector3::UNIT_Y);
					}
					else if(fionaConf.currentJoystick.x > 0.f)
					{
						qMult.FromAngleAxis(Ogre::Radian::Radian(0.05f), Ogre::Vector3::UNIT_Y);
					}
					Ogre::Quaternion qNewRot = qRot * qMult;
					pObj->getParentSceneNode()->setOrientation(qNewRot);
				}
			}
		}
	}
	else
	{
		if(wbScene->getCurrentTool() == WorldBuilderScene::SCULPTER)
		{
			if(wbScene->getAddMode() == WorldBuilderScene::CUBE)
			{
				jvec3 vAddCube = wbScene->getAddCube();
				vAddCube.x += 0.001 * fionaConf.currentJoystick.x;
				vAddCube.y += 0.001 * fionaConf.currentJoystick.z;
				wbScene->setAddCube(vAddCube);
			}
			else if(wbScene->getAddMode() == WorldBuilderScene::BLOB)
			{
				//adjust flow control..
				float flowMultiplier = wbScene->getFlowMultiplier();
				flowMultiplier += fionaConf.currentJoystick.z*5.f;
				if(flowMultiplier < WorldBuilderScene::MINIMUM_FLOW_SIZE)
				{
					flowMultiplier = WorldBuilderScene::MINIMUM_FLOW_SIZE;
				}
				wbScene->setFlowMultiplier(flowMultiplier);
			}
		}
		else if(wbScene->getCurrentTool() == WorldBuilderScene::ERASER)
		{
			float beamRadius = wbScene->getBeamRadius();
			if(beamRadius > 0.01f)
			{
				beamRadius += 0.001 * fionaConf.currentJoystick.x;
				wbScene->setBeamRadius(beamRadius);
			}

			float beamLength = wbScene->getBeamLength();
			if(beamLength > 0.01f)
			{
				beamLength += 0.001 * fionaConf.currentJoystick.z;
				wbScene->setBeamLength(beamLength);
			}
		}
		else if(wbScene->getCurrentTool() == WorldBuilderScene::PAINT)
		{
			float flowMultiplier = wbScene->getFlowMultiplier();
			flowMultiplier += fionaConf.currentJoystick.z*5.f;
			if(flowMultiplier < WorldBuilderScene::MINIMUM_FLOW_SIZE)
			{
				flowMultiplier = WorldBuilderScene::MINIMUM_FLOW_SIZE;
			}
			wbScene->setFlowMultiplier(flowMultiplier);
		}
	}
}

void VRPaint::ButtonDown(void)
{
	WandMove();
}

void VRPaint::WandMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	jvec3 vWandDir;
	wbScene->getWandDirWorldSpace(vWandDir, true, wbScene->getWiiFitRotation());
	vWandDir *= WorldBuilderScene::WAND_OFFSET_DISTANCE;
	jvec3 pos = wbScene->getNormalizedWandPos(vWandDir.x, vWandDir.y, vWandDir.z);
	float rad = fionaConf.emgAvg;
	if (rad <= 0.0) { rad = 100.0; }

	rad *= wbScene->getFlowMultiplier();

	if(WorldBuilderScene::inBounds(pos))
	{
		float diff = fionaConf.physicsStep * 0.001f;
		const jvec3 &currColor = wbScene->getCurrentColor();
		wbScene->getSystem()->paint(pos.x, pos.y, pos.z, currColor.x, currColor.y, currColor.z, rad * WorldBuilderScene::EMG_MULTIPLIER * diff);
	}
}

void VRPaint::ButtonUp(void)
{

}

void VRPaint::JoystickMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	float flowMultiplier = wbScene->getFlowMultiplier();
	flowMultiplier += fionaConf.currentJoystick.z*5.f;
	if(flowMultiplier < WorldBuilderScene::MINIMUM_FLOW_SIZE)
	{
		flowMultiplier = WorldBuilderScene::MINIMUM_FLOW_SIZE;
	}
	wbScene->setFlowMultiplier(flowMultiplier);
}

void VRPaint::DrawCallback(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	jvec3 vWandDir;
	wbScene->getWandDirWorldSpace(vWandDir, true, wbScene->getWiiFitRotation());
	vWandDir *= WorldBuilderScene::WAND_OFFSET_DISTANCE;

	jvec3 pos = wbScene->getWandPos();
	pos.x += vWandDir.x;
	pos.y += vWandDir.y;
	pos.z += vWandDir.z;

	jvec3 currColor = wbScene->getCurrentColor();
	float flow = wbScene->getFlowMultiplier();
	float radius = flow * 0.03f / 200.f;
	//float radius = 0.03f;
	
	glPushMatrix();
	glDisable(GL_LIGHTING);
	glTranslatef(pos.x, pos.y, pos.z);
	glColor3f(currColor.x, currColor.y, currColor.z);
	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	glPointSize(10.f);
	glutSolidSphere(radius, 24, 24);
	glPointSize(1.f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void VRResetSculpt::ButtonDown(void)
{
	m_timeDown = FionaUTTime();
	m_bReset = false;
}

void VRResetSculpt::WandMove(void)
{
	if (m_bReset) { return; }

	float currTime = FionaUTTime();
	if(currTime - m_timeDown >= 3.f)
	{
		WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
		wbScene->getSystem()->reset();
		if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
			wbScene->playSound("delete");
		}
		m_bReset = true;
		return;
	} 
}

void VRResetSculpt::ButtonUp(void)
{
    if (!m_bReset) 
	{
        WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
        //wbScene->getSystem()->undo();
    }

    m_timeDown = 0.f;
    m_bReset = false;
}

void VRRotate::ButtonDown(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
	m_bDoRotate = false;
	
	if(wbScene->numSelected() > 0)
	{
		float fDistance = 0.f;

		Ogre::MovableObject *pObj = wbScene->rayCastSelect(fDistance);
		if(pObj)
		{
			jvec3 vPos;
			wbScene->getWandWorldSpace(vPos, true);
			//this correctly orients the direction of fire..
			jvec3 vWandDir;
			wbScene->getWandDirWorldSpace(vWandDir, true);
			jvec3 vNewPos = vPos + vWandDir * fDistance;

			m_fSelectionDistance = fDistance;
			Ogre::Vector3 vIntersectPos(vNewPos.x, vNewPos.y, vNewPos.z);
			Ogre::Vector3 vec = pObj->getParentSceneNode()->getPosition();
			Ogre::Vector3 vOffset = vec - vIntersectPos;
			m_vOffset.set(vOffset.x, vOffset.y, vOffset.z);
			m_bDoRotate = true;
		}
	}
}

void VRRotate::ButtonUp(void)
{
	m_bDoRotate = false;
	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f ,0.f, 0.f);
}

void VRRotate::WandMove(void)
{
	if(m_bDoRotate)
	{
		//todo - eventually handle multiple selection..
		WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
		if(wbScene->numSelected() > 0)
		{
			Ogre::MovableObject *pObj = wbScene->getSelected(0);
			if(pObj)
			{
				jvec3 vPos;
				wbScene->getWandWorldSpace(vPos, true);
				//this correctly orients the direction of fire..
				jvec3 vWandDir;
				wbScene->getWandDirWorldSpace(vWandDir, true);

				jvec3 vNewPos = m_vOffset + vPos + vWandDir * m_fSelectionDistance;
				pObj->getParentSceneNode()->setPosition(vNewPos.x, vNewPos.y, vNewPos.z);
			}
		}
	}
}

void VRRotate::JoystickMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->numSelected() > 0)
	{
		Ogre::MovableObject *pObj = wbScene->getSelected(0);
		if(pObj)
		{
			const Ogre::Quaternion &qRot = pObj->getParentSceneNode()->getOrientation();
			Ogre::Quaternion qMult = Ogre::Quaternion::IDENTITY;
			if(fionaConf.currentJoystick.x < 0.f)
			{
				qMult.FromAngleAxis(Ogre::Radian::Radian(-0.05f), Ogre::Vector3::UNIT_Y);
			}
			else if(fionaConf.currentJoystick.x > 0.f)
			{
				qMult.FromAngleAxis(Ogre::Radian::Radian(0.05f), Ogre::Vector3::UNIT_Y);
			}
			else if(fionaConf.currentJoystick.z < 0.f)
			{
				qMult.FromAngleAxis(Ogre::Radian::Radian(-0.05f), Ogre::Vector3::UNIT_X);
			}
			else if(fionaConf.currentJoystick.z > 0.f)
			{
				qMult.FromAngleAxis(Ogre::Radian::Radian(0.05f), Ogre::Vector3::UNIT_X);
			}
			Ogre::Quaternion qNewRot = qRot * qMult;
			pObj->getParentSceneNode()->setOrientation(qNewRot);
		}
	}
}

void VRSave::ButtonUp(void)
{
	if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) 
	{
		WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
		wbScene->saveScene();
	}
}

void VRScale::ButtonDown(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	m_bDoScale = false;
	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
	
	float fDist = 0.f;
	Ogre::MovableObject *pObj = wbScene->rayCastSelect(fDist);
	if(pObj)
	{
		jvec3 vPos;
		wbScene->getWandWorldSpace(vPos, true);
		//this correctly orients the direction of fire..
		jvec3 vWandDir;
		wbScene->getWandDirWorldSpace(vWandDir, true);

		m_fSelectionDistance = fDist;
		jvec3 vNewPos = vPos + vWandDir * fDist;
		Ogre::Vector3 vIntersectPos(vNewPos.x, vNewPos.y, vNewPos.z);
		Ogre::Vector3 vec = pObj->getParentSceneNode()->getPosition();
		Ogre::Vector3 vOffset = vec - vIntersectPos;
		m_vOffset.set(vOffset.x, vOffset.y, vOffset.z);
		m_bDoScale = true;
	}
}

void VRScale::ButtonUp(void)
{
	m_bDoScale = false;
	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
}

void VRScale::WandMove(void)
{
	if(m_bDoScale)
	{
		//todo - eventually handle multiple selection..
		WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
		if(wbScene->numSelected() > 0)
		{
			Ogre::MovableObject *pObj = wbScene->getSelected(0);
			if(pObj)
			{
				jvec3 vPos;
				wbScene->getWandWorldSpace(vPos, true);
				//this correctly orients the direction of fire..
				jvec3 vWandDir;
				wbScene->getWandDirWorldSpace(vWandDir, true);

				jvec3 vNewPos = m_vOffset + vPos + vWandDir * m_fSelectionDistance;
				pObj->getParentSceneNode()->setPosition(vNewPos.x, vNewPos.y, vNewPos.z);
			}
		}
	}
}

void VRScale::JoystickMove(void)
{
	if(m_bDoScale)
	{
		WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
		Ogre::MovableObject *pObj = wbScene->getSelected(0);
		if(pObj)
		{
			Ogre::Vector3 vScale = pObj->getParentSceneNode()->getScale();
			//printf("curr joystick: %f, %f\n", fionaConf.currentJoystick.x, fionaConf.currentJoystick.z);
			if(fionaConf.currentJoystick.x > 0.f)
			{
				vScale.x += 0.005f;
				vScale.y += 0.005f;
				vScale.z += 0.005f;
				pObj->getParentSceneNode()->setScale(vScale);
			}
			else if(fionaConf.currentJoystick.x < 0.f)
			{
				vScale.x -= 0.005f;
				vScale.y -= 0.005f;
				vScale.z -= 0.005f;
				pObj->getParentSceneNode()->setScale(vScale);
			}
		}
	}
}

#define USE_LEAP 0
void VRSculpt::ButtonDown(void)
{
	//this could someday begin an undo operation..
	WandMove();
}

void VRSculpt::WandMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	float rad = fionaConf.emgAvg;
	if (rad <= 0.0) { rad = 100.0; }

	rad *= wbScene->getFlowMultiplier();

	jvec3 trackerPos;
	
	if(!m_bUseSecondTracker)
	{
		//trying offset of indicator..
		jvec3 vWandDir;
#if USE_LEAP
		if(fionaConf.leapData.hand1.valid)
		{
			trackerPos.x = fionaConf.leapData.hand1.handPosition[0];
			trackerPos.y = fionaConf.leapData.hand1.handPosition[1];
			trackerPos.z = fionaConf.leapData.hand1.handPosition[2];

			wbScene->toWorldSpace(trackerPos, false);

			//printf("%f, %f, %f\n", pos.x, pos.y, pos.z);
			vWandDir.x = fionaConf.leapData.hand1.handDirection[0];
			vWandDir.y = fionaConf.leapData.hand1.handDirection[1];
			vWandDir.z = fionaConf.leapData.hand1.handDirection[2];
			vWandDir *= WorldBuilderScene::WAND_OFFSET_DISTANCE;

			trackerPos = wbScene->getNormalizedPos(trackerPos);
		}
		else
		{
			trackerPos.set(0.f, 0.f, 0.f);
		}
#else
		wbScene->getWandDirWorldSpace(vWandDir, true);// wbScene->getWiiFitRotation());
		vWandDir *= WorldBuilderScene::WAND_OFFSET_DISTANCE;
		trackerPos = wbScene->getNormalizedWandPos(vWandDir.x, vWandDir.y, vWandDir.z);
#endif
	}
	else
	{
		wbScene->getNormalizedSecondTrackerPos(trackerPos);
	}

	if(!m_bUseSecondTracker || fionaConf.emgBinary)
	{
		if(WorldBuilderScene::inBounds(trackerPos)) 
		{
			if (wbScene->getAddMode() == WorldBuilderScene::BLOB) 
			{
				float diff = fionaConf.physicsStep * 0.001f;
				const jvec3 &vColor = wbScene->getCurrentColor();
				float cubeScale = wbScene->getCubeScale();
				wbScene->getSystem()->add(trackerPos.x, trackerPos.y, trackerPos.z, rad * WorldBuilderScene::EMG_MULTIPLIER * diff, cubeScale, 0, vColor.x, vColor.y, vColor.z);
			} 
			else if (wbScene->getAddMode() == WorldBuilderScene::CUBE)
			{
				//only add one cube each time the emg is triggered...
				//if(!m_bCubeAdded)
				//{
					jvec3 vWandDir;
					wbScene->getWandDirWorldSpace(vWandDir, true);//, wbScene->getWiiFitRotation());
					vWandDir *= WorldBuilderScene::WAND_OFFSET_DISTANCE;

					jvec3 addCube = wbScene->getAddCube();
					jvec3 tmp = addCube / 2.0f;
					
					/*float toDeg = wbScene->getWiiFitRotation() * PI/180;
					mat3 m(cosf(toDeg), 0.f, -sinf(toDeg), 0.f, 1.f, 0.f, sinf(toDeg), 0.f, cosf(toDeg));
					tmp = m * tmp;*/

					jvec3 vTmp1 = -tmp + vWandDir;
					jvec3 vTmp2 = tmp + vWandDir;
					m_addCubeP1 = wbScene->getNormalizedWandPos(vTmp1.x, vTmp1.y, vTmp1.z);
					m_addCubeP2 = wbScene->getNormalizedWandPos(vTmp2.x, vTmp2.y, vTmp2.z);
				
					const jvec3 &vColor = wbScene->getCurrentColor();
					wbScene->getSystem()->addCube(  m_addCubeP1.x, m_addCubeP1.y, m_addCubeP1.z, m_addCubeP2.x, m_addCubeP2.y, m_addCubeP2.z, vColor.x, vColor.y, vColor.z);
				//	m_bCubeAdded = true;
				//}
			}
		}
	}
}

void VRSculpt::ButtonUp(void)
{
	m_bCubeAdded = false;
}

void VRSculpt::JoystickMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->getAddMode() == WorldBuilderScene::CUBE)
	{
		jvec3 vAddCube = wbScene->getAddCube();
		vAddCube.x += 0.001 * fionaConf.currentJoystick.x;
		vAddCube.y += 0.001 * fionaConf.currentJoystick.z;
		wbScene->setAddCube(vAddCube);
	}
	else
	{
		float flowMultiplier = wbScene->getFlowMultiplier();
		flowMultiplier += fionaConf.currentJoystick.z*5.f;
		if(flowMultiplier < WorldBuilderScene::MINIMUM_FLOW_SIZE)
		{
			flowMultiplier = WorldBuilderScene::MINIMUM_FLOW_SIZE;
		}
		wbScene->setFlowMultiplier(flowMultiplier);
	}
}


void VRSculpt::DrawCallback(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	jvec3 vWandDir;
	wbScene->getWandDirWorldSpace(vWandDir, true, wbScene->getWiiFitRotation());
	vWandDir *= WorldBuilderScene::WAND_OFFSET_DISTANCE;

	jvec3 pos;
	jvec3 vNormPos;
	if(m_bUseSecondTracker)
	{
		wbScene->getSecondTrackerPos(pos);
		wbScene->getNormalizedSecondTrackerPos(vNormPos);
	}
	else
	{
#if USE_LEAP
		if(fionaConf.leapData.hand1.valid)
		{
			pos.x = fionaConf.leapData.hand1.handPosition[0];
			pos.y = fionaConf.leapData.hand1.handPosition[1];
			pos.z = fionaConf.leapData.hand1.handPosition[2];

			wbScene->toWorldSpace(pos, false);

			//printf("%f, %f, %f\n", pos.x, pos.y, pos.z);
			vWandDir.x = fionaConf.leapData.hand1.handDirection[0];
			vWandDir.y = fionaConf.leapData.hand1.handDirection[1];
			vWandDir.z = fionaConf.leapData.hand1.handDirection[2];
			vWandDir *= WorldBuilderScene::WAND_OFFSET_DISTANCE;

			vNormPos = wbScene->getNormalizedPos(pos);
		}
		else
		{
			pos.set(0.f, 0.f, 0.f);
			vNormPos.set(0.f, 0.f, 0.f);
		}
#else
		pos = wbScene->getWandPos();
		vNormPos = wbScene->getNormalizedWandPos(vWandDir.x, vWandDir.y, vWandDir.z);
#endif
	}
	
	if(WorldBuilderScene::inBounds(vNormPos))
	{
		const jvec3 &vColor = wbScene->getCurrentColor();
		
		if(!m_bUseSecondTracker)
		{
			pos.x += vWandDir.x;
			pos.y += vWandDir.y;
			pos.z += vWandDir.z;
		}

		if (wbScene->getAddMode() == WorldBuilderScene::BLOB)
		{
#if USE_LEAP
			float radius = 0.f;
			if(fionaConf.leapData.hand1.valid)
			{
				radius = fionaConf.leapData.hand1.sphereRadius;
			}
#else
			float flow = wbScene->getFlowMultiplier();
			float radius = flow * 0.03f / 200.f;
#endif
			glPushMatrix();
			glDisable(GL_LIGHTING);
			//printf("%f %f %f\n", pos.x, pos.y, pos.z);
			glTranslatef(pos.x, pos.y, pos.z);
			glColor3f(vColor.x, vColor.y, vColor.z);
			glutWireSphere(radius, 6, 6);
			glEnable(GL_LIGHTING);
			glPopMatrix();
		}
		else if(wbScene->getAddMode() == WorldBuilderScene::CUBE)
		{
			const jvec3 &addCube = wbScene->getAddCube();

			glPushMatrix();
			glDisable(GL_LIGHTING);
			glTranslatef(pos.x, pos.y, pos.z);
			glColor3f(vColor.x, vColor.y, vColor.z);
			glScalef( addCube.x, addCube.y, addCube.z);
			glutWireCube(1.0);
			glEnable(GL_LIGHTING);
			glPopMatrix();
		}
	}
}

void VRSelect::ButtonUp(void)
{
	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
	m_bDoTranslate = false;
}

void VRSelect::ButtonDown(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
	m_bDoTranslate = false;
	
	float fDistance = 0.f;

	Ogre::MovableObject *pObj = wbScene->rayCastSelect(fDistance);
	if(pObj)
	{
		jvec3 vPos;
		wbScene->getWandWorldSpace(vPos, true);
		//this correctly orients the direction of fire..
		jvec3 vWandDir;
		wbScene->getWandDirWorldSpace(vWandDir, true);
		jvec3 vNewPos = vPos + vWandDir * fDistance;

		m_fSelectionDistance = fDistance;
		Ogre::Vector3 vIntersectPos(vNewPos.x, vNewPos.y, vNewPos.z);
		Ogre::Vector3 vec = pObj->getParentSceneNode()->getPosition();
		Ogre::Vector3 vOffset = vec - vIntersectPos;
		m_vOffset.set(vOffset.x, vOffset.y, vOffset.z);
		m_bDoTranslate = true;
	}
}

void VRSelect::WandMove(void)
{
	if(m_bDoTranslate)
	{
		//todo - eventually handle multiple selection..
		WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
		if(wbScene->numSelected() > 0)
		{
			Ogre::MovableObject *pObj = wbScene->getSelected(0);
			if(pObj)
			{
				jvec3 vPos;
				wbScene->getWandWorldSpace(vPos, true);
				//this correctly orients the direction of fire..
				jvec3 vWandDir;
				wbScene->getWandDirWorldSpace(vWandDir, true);
				vWandDir = vWandDir.normalize();

				jvec3 vWandUp;
				wbScene->getWandDirWorldSpace(vWandUp, false);
				vWandUp = vWandUp.normalize();

				jvec3 vWandLeft = vWandDir * vWandUp;
				vWandLeft = vWandLeft.normalize();

				/*Ogre::Matrix3 mat;
				mat.SetColumn(0, Ogre::Vector3(vWandDir.x, vWandDir.y, vWandDir.z));
				mat.SetColumn(1, Ogre::Vector3(vWandUp.x, vWandUp.y, vWandUp.z));
				mat.SetColumn(2, Ogre::Vector3(vWandLeft.x, vWandLeft.y, vWandLeft.z));*/

				jvec3 vNewPos = m_vOffset + (vPos + vWandDir * m_fSelectionDistance);
				pObj->getParentSceneNode()->setPosition(vNewPos.x, vNewPos.y, vNewPos.z);
				//pObj->getParentSceneNode()->setOrientation(mat);
			}
		}
	}
}

void VRSelect::JoystickMove(void)
{
	if(m_bDoTranslate)
	{
		WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
		if(wbScene->numSelected() > 0)
		{
			Ogre::MovableObject *pObj = wbScene->getSelected(0);
			if(pObj)
			{
				if(wbScene->getTSR() == WorldBuilderScene::TRANSLATE)
				{
					m_fSelectionDistance += (fionaConf.currentJoystick.z * 0.1f);
				}
				else if(wbScene->getTSR() == WorldBuilderScene::SCALE)
				{
					Ogre::MovableObject *pObj = wbScene->getSelected(0);
					if(pObj)
					{
						Ogre::Vector3 vScale = pObj->getParentSceneNode()->getScale();
						//printf("curr joystick: %f, %f\n", fionaConf.currentJoystick.x, fionaConf.currentJoystick.z);
						if(fionaConf.currentJoystick.x > 0.f)
						{
							vScale.x += 0.005f;
							vScale.y += 0.005f;
							vScale.z += 0.005f;
							pObj->getParentSceneNode()->setScale(vScale);
						}
						else if(fionaConf.currentJoystick.x < 0.f)
						{
							vScale.x -= 0.005f;
							vScale.y -= 0.005f;
							vScale.z -= 0.005f;
							pObj->getParentSceneNode()->setScale(vScale);
						}
					}
				}
				else if(wbScene->getTSR() == WorldBuilderScene::ROTATE)
				{
					Ogre::MovableObject *pObj = wbScene->getSelected(0);
					if(pObj)
					{
						const Ogre::Quaternion &qRot = pObj->getParentSceneNode()->getOrientation();
						Ogre::Quaternion qMult = Ogre::Quaternion::IDENTITY;
						if(fionaConf.currentJoystick.x < 0.f)
						{
							qMult.FromAngleAxis(Ogre::Radian::Radian(-0.05f), Ogre::Vector3::UNIT_Y);
						}
						else if(fionaConf.currentJoystick.x > 0.f)
						{
							qMult.FromAngleAxis(Ogre::Radian::Radian(0.05f), Ogre::Vector3::UNIT_Y);
						}
						Ogre::Quaternion qNewRot = qRot * qMult;
						pObj->getParentSceneNode()->setOrientation(qNewRot);
					}
				}
			}
		}
	}
}

void VRSwitchModes::ButtonUp(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	wbScene->changeModes(!wbScene->isWorldMode());
}

void VRTranslate::ButtonDown(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
	m_bDoTranslate = false;
	
	if(wbScene->numSelected() > 0)
	{
		float fDistance = 0.f;

		Ogre::MovableObject *pObj = wbScene->rayCastSelect(fDistance);
		if(pObj)
		{
			jvec3 vPos;
			wbScene->getWandWorldSpace(vPos, true);
			//this correctly orients the direction of fire..
			jvec3 vWandDir;
			wbScene->getWandDirWorldSpace(vWandDir, true);
			jvec3 vNewPos = vPos + vWandDir * fDistance;

			m_fSelectionDistance = fDistance;
			Ogre::Vector3 vIntersectPos(vNewPos.x, vNewPos.y, vNewPos.z);
			Ogre::Vector3 vec = pObj->getParentSceneNode()->getPosition();
			Ogre::Vector3 vOffset = vec - vIntersectPos;
			m_vOffset.set(vOffset.x, vOffset.y, vOffset.z);
			m_bDoTranslate = true;
		}
	}
}

void VRTranslate::WandMove(void)
{
	if(m_bDoTranslate)
	{
		//todo - eventually handle multiple selection..
		WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
		if(wbScene->numSelected() > 0)
		{
			Ogre::MovableObject *pObj = wbScene->getSelected(0);
			if(pObj)
			{
				jvec3 vPos;
				wbScene->getWandWorldSpace(vPos, true);
				//this correctly orients the direction of fire..
				jvec3 vWandDir;
				wbScene->getWandDirWorldSpace(vWandDir, true);

				jvec3 vNewPos = m_vOffset + (vPos + vWandDir * m_fSelectionDistance);
				pObj->getParentSceneNode()->setPosition(vNewPos.x, vNewPos.y, vNewPos.z);
			}
		}
	}
}

void VRTranslate::JoystickMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);
	if(wbScene->numSelected() > 0)
	{
		m_fSelectionDistance += (fionaConf.currentJoystick.z * 0.1f);
	}
}

void VRTranslate::ButtonUp(void)
{
	m_bDoTranslate = false;
	m_fSelectionDistance = 0.f;
	m_vOffset.set(0.f, 0.f, 0.f);
}


void VRUseTool::ButtonDown(void)
{
	//this could someday begin an undo operation..
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->getCurrentTool() == WorldBuilderScene::SCULPTER)
	{
		m_sculptAction.SetScenePtr(wbScene);
		m_sculptAction.ButtonDown();
	}
	else if(wbScene->getCurrentTool() == WorldBuilderScene::ERASER)
	{
		m_eraseAction.SetScenePtr(wbScene);
		m_eraseAction.ButtonDown();
	}
	else if(wbScene->getCurrentTool() == WorldBuilderScene::PAINT)
	{
		m_paintAction.SetScenePtr(wbScene);
		m_paintAction.ButtonDown();
	}
}

void VRUseTool::WandMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->getCurrentTool() == WorldBuilderScene::SCULPTER)
	{
		m_sculptAction.SetScenePtr(wbScene);
		m_sculptAction.WandMove();
	}
	else if(wbScene->getCurrentTool() == WorldBuilderScene::ERASER)
	{
		m_eraseAction.SetScenePtr(wbScene);
		m_eraseAction.WandMove();
	}
	else if(wbScene->getCurrentTool() == WorldBuilderScene::PAINT)
	{
		m_paintAction.SetScenePtr(wbScene);
		m_paintAction.WandMove();
	}
}

void VRUseTool::ButtonUp(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->getCurrentTool() == WorldBuilderScene::SCULPTER)
	{
		m_sculptAction.SetScenePtr(wbScene);
		m_sculptAction.ButtonUp();
	}
	else if(wbScene->getCurrentTool() == WorldBuilderScene::ERASER)
	{
		m_eraseAction.SetScenePtr(wbScene);
		m_eraseAction.ButtonUp();
	}
	else if(wbScene->getCurrentTool() == WorldBuilderScene::PAINT)
	{
		m_paintAction.SetScenePtr(wbScene);
		m_paintAction.ButtonUp();
	}
}

void VRUseTool::JoystickMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->getCurrentTool() == WorldBuilderScene::SCULPTER)
	{
		m_sculptAction.SetScenePtr(wbScene);
		m_sculptAction.JoystickMove();
	}
	else if(wbScene->getCurrentTool() == WorldBuilderScene::ERASER)
	{
		m_eraseAction.SetScenePtr(wbScene);
		m_eraseAction.JoystickMove();
	}
	else if(wbScene->getCurrentTool() == WorldBuilderScene::PAINT)
	{
		m_paintAction.SetScenePtr(wbScene);
		m_paintAction.JoystickMove();
	}
}

void VRUseTool::DrawCallback(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->getCurrentTool() == WorldBuilderScene::SCULPTER)
	{
		m_sculptAction.SetScenePtr(wbScene);
		m_sculptAction.DrawCallback();
	}
	else if(wbScene->getCurrentTool() == WorldBuilderScene::ERASER)
	{
		m_eraseAction.SetScenePtr(wbScene);
		m_eraseAction.DrawCallback();
	}
	else if(wbScene->getCurrentTool() == WorldBuilderScene::PAINT)
	{
		m_paintAction.SetScenePtr(wbScene);
		m_paintAction.DrawCallback();
	}
}

void VRTSR::ButtonDown(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->getTSR() == WorldBuilderScene::TRANSLATE)
	{
		m_translateAction.SetScenePtr(wbScene);
		m_translateAction.ButtonDown();
	}
	else if(wbScene->getTSR() == WorldBuilderScene::ROTATE)
	{
		m_rotateAction.SetScenePtr(wbScene);
		m_rotateAction.ButtonDown();
	}
	else if(wbScene->getTSR() == WorldBuilderScene::SCALE)
	{
		m_scaleAction.SetScenePtr(wbScene);
		m_scaleAction.ButtonDown();
	}
}

void VRTSR::WandMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->getTSR() == WorldBuilderScene::TRANSLATE)
	{
		m_translateAction.SetScenePtr(wbScene);
		m_translateAction.WandMove();
	}
	else if(wbScene->getTSR() == WorldBuilderScene::ROTATE)
	{
		m_rotateAction.SetScenePtr(wbScene);
		m_rotateAction.WandMove();
	}
	else if(wbScene->getTSR() == WorldBuilderScene::SCALE)
	{
		m_scaleAction.SetScenePtr(wbScene);
		m_scaleAction.WandMove();
	}
}

void VRTSR::ButtonUp(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->getTSR() == WorldBuilderScene::TRANSLATE)
	{
		m_translateAction.SetScenePtr(wbScene);
		m_translateAction.ButtonUp();
	}
	else if(wbScene->getTSR() == WorldBuilderScene::ROTATE)
	{
		m_rotateAction.SetScenePtr(wbScene);
		m_rotateAction.ButtonUp();
	}
	else if(wbScene->getTSR() == WorldBuilderScene::SCALE)
	{
		m_scaleAction.SetScenePtr(wbScene);
		m_scaleAction.ButtonUp();
	}
}

void VRTSR::JoystickMove(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->getTSR() == WorldBuilderScene::TRANSLATE)
	{
		m_translateAction.SetScenePtr(wbScene);
		m_translateAction.JoystickMove();
	}
	else if(wbScene->getTSR() == WorldBuilderScene::ROTATE)
	{
		m_rotateAction.SetScenePtr(wbScene);
		m_rotateAction.JoystickMove();
	}
	else if(wbScene->getTSR() == WorldBuilderScene::SCALE)
	{
		m_scaleAction.SetScenePtr(wbScene);
		m_scaleAction.JoystickMove();
	}
}

void VRTSR::DrawCallback(void)
{
	WorldBuilderScene *wbScene = static_cast<WorldBuilderScene*>(m_scene);

	if(wbScene->getTSR() == WorldBuilderScene::TRANSLATE)
	{
		m_translateAction.SetScenePtr(wbScene);
		m_translateAction.DrawCallback();
	}
	else if(wbScene->getTSR() == WorldBuilderScene::ROTATE)
	{
		m_rotateAction.SetScenePtr(wbScene);
		m_rotateAction.DrawCallback();
	}
	else if(wbScene->getTSR() == WorldBuilderScene::SCALE)
	{
		m_scaleAction.SetScenePtr(wbScene);
		m_scaleAction.DrawCallback();
	}
}
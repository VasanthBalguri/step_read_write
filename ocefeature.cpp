#include "ocefeature.h"

void OceShape::generateMesh()
{
    TopAbs_ShapeEnum shapetype = this->getShapeType();
    osg::ref_ptr<osg::Vec3Array> vertexList = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> colorList = new osg::Vec3Array();
    unsigned int noOfTriangles = 0;
    // clean any previous triangulation
    BRepTools::Clean(_shape);

    //_healShape(shape);

    /// call to incremental mesh on this shape
    /// \todo not sure why this 1 is passed. Its called deflection BTW
    ///       need to find a way to calculate it
    double linearDeflection = 1.0;
    BRepMesh_IncrementalMesh(_shape, linearDeflection);

    switch(shapetype)
    {
    case TopAbs_EDGE:case TopAbs_WIRE:
    {
        osg::ref_ptr<osg::DrawElementsUInt> lineStrip = new osg::DrawElementsUInt(osg::PrimitiveSet::LINE_STRIP, 0);
        for(TopExp_Explorer ex(_shape, TopAbs_EDGE);ex.More();ex.Next())
        {
            TopoDS_Edge edge = TopoDS::Edge(ex.Current());
            TopLoc_Location location;

            Handle(Poly_Polygon3D) edgeMesh = BRep_Tool::Polygon3D(edge,location);
            if(!edgeMesh.IsNull())
            {
                int noOfNodes = edgeMesh->NbNodes();

                for(int j= 1; j <= noOfNodes; j++)
                {
                    gp_Pnt pt = (edgeMesh->Nodes())(j).Transformed(_transform * location.Transformation());
                    vertexList->push_back(osg::Vec3(pt.X(), pt.Y(), pt.Z()));
                    lineStrip->push_back(j-1);
                }
            }

        }

        _geom->setVertexArray(vertexList.get());
        _geom->addPrimitiveSet(lineStrip.get());

    }
    case TopAbs_FACE:case TopAbs_SHELL:case TopAbs_SOLID:
    {
        ///iterate faces
        // this variable will help in keeping track of face indices
        unsigned int index = 0;
        osg::ref_ptr<osg::DrawElementsUInt> triangleStrip = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
        for (TopExp_Explorer ex(_shape, TopAbs_FACE); ex.More(); ex.Next())
        {
            TopoDS_Face face = TopoDS::Face(ex.Current());
            TopLoc_Location location;

            /// triangulate current face
            Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
            if (!triangulation.IsNull())
            {
                int noOfNodes = triangulation->NbNodes();

                // Store vertices. Build vertex array here
                for(int j = 1; j <= triangulation->NbNodes(); j++)
                {
                    // populate vertex list
                    // Ref: http://www.opencascade.org/org/forum/thread_16694/?forum=3
                    gp_Pnt pt = (triangulation->Nodes())(j).Transformed(_transform * location.Transformation());
                    vertexList->push_back(osg::Vec3(pt.X(), pt.Y(), pt.Z()));
                }

                /// now we need to get face indices for triangles
                // get list of triangle first
                const Poly_Array1OfTriangle& triangles = triangulation->Triangles();

                //No of triangles in this triangulation
                noOfTriangles = triangulation->NbTriangles();

                Standard_Integer v1, v2, v3;
                for (unsigned int j = 1; j <= noOfTriangles; j++)
                {
                    /// If face direction is reversed then we add verticews in reverse order
                    /// order of vertices is important for normal calculation later
                    if (face.Orientation() == TopAbs_REVERSED)
                    {
                        triangles(j).Get(v1, v3, v2);
                    }
                    else
                    {
                        triangles(j).Get(v1, v2, v3);
                    }

                    triangleStrip->push_back(index + v1 - 1);
                    triangleStrip->push_back(index + v2 - 1);
                    triangleStrip->push_back(index + v3 - 1);
                }
                index = index + noOfNodes;
            }
        }
        _geom->setVertexArray(vertexList.get());
        _geom->addPrimitiveSet(triangleStrip.get());
    }
    case TopAbs_VERTEX:
    {
        int index = 0;
        osg::ref_ptr<osg::DrawElementsUInt> points = new osg::DrawElementsUInt(osg::PrimitiveSet::POINTS, 0);
        for (TopExp_Explorer ex(_shape, TopAbs_VERTEX); ex.More(); ex.Next())
        {
            TopoDS_Vertex vertex = TopoDS::Vertex(ex.Current());
            gp_XYZ location = vertex.Location().Transformation().TranslationPart();
            vertexList->push_back(osg::Vec3(location.X(),location.Y(),location.Z()));
            points->push_back(index);
            index++;
        }
        _geom->setVertexArray(vertexList.get());
        _geom->addPrimitiveSet(points.get());
    }
    }


    colorList->push_back(_color);
osg::Array::Binding colorBinding = osg::Array::BIND_OVERALL;

    _geom->setColorArray(colorList.get(), colorBinding);

    osgUtil::SmoothingVisitor::smooth(*(_geom),osg::PI/6.0);
    this->addDrawable(_geom);

    osg::ref_ptr<osg::StateSet> stateSet = this->getOrCreateStateSet();
    osg::ref_ptr<osg::Material> material = new osg::Material;
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    stateSet->setAttributeAndModes( material, osg::StateAttribute::ON );
    stateSet->setMode( GL_DEPTH_TEST, osg::StateAttribute::ON );
}

void StepFileRW::readFile(std::string path)
{
    STEPControl_Reader aReader;
    TopoDS_Shape aShape;

_shapeTree = new osg::MatrixTransform;

    if(aReader.ReadFile(path.c_str()) != IFSelect_RetDone)
        return;

    Standard_Integer nbr = aReader.NbRootsForTransfer();

    for (Standard_Integer n = 1; n<= nbr; n++) {
        aReader.TransferRoot(n);
    }

    Standard_Integer nbs = aReader.NbShapes();

    if(nbs != 0)
    {
        for (Standard_Integer i=1; i<=nbs; i++) {
            aShape = aReader.Shape(i);

        TopExp_Explorer explorer;
        for(explorer.Init(aShape,TopAbs_SOLID);explorer.More();explorer.Next())
        {
            TopoDS_Shape s = explorer.Current();
            osg::ref_ptr<OceShape> solid = new OceShape(s);

            osg::ref_ptr<osg::Switch> s_solid = new osg::Switch;
            osg::ref_ptr<osg::MatrixTransform> t_solid = new osg::MatrixTransform;
            osg::Matrix m;
            solid->generateMesh();
            m.setTrans(osg::Vec3(solid->getTopoShape().Location().Transformation().TranslationPart().X(),
                                 solid->getTopoShape().Location().Transformation().TranslationPart().Y(),
                                 solid->getTopoShape().Location().Transformation().TranslationPart().Z()));
            m.setRotate(osg::Quat(solid->getTopoShape().Location().Transformation().GetRotation().X(),
                        solid->getTopoShape().Location().Transformation().GetRotation().Y(),
                        solid->getTopoShape().Location().Transformation().GetRotation().Z(),
                        solid->getTopoShape().Location().Transformation().GetRotation().W()));
            t_solid->setMatrix(m);


            t_solid->addChild(solid);
            s_solid->addChild(t_solid);
            _shapeTree->addChild(s_solid);
        }
        for(explorer.Init(aShape,TopAbs_SHELL,TopAbs_SOLID);explorer.More();explorer.Next())
        {
            TopoDS_Shape s = explorer.Current();
            osg::ref_ptr<OceShape> shell = new OceShape(s);

            osg::ref_ptr<osg::Switch> s_shell = new osg::Switch;
            osg::ref_ptr<osg::MatrixTransform> t_shell= new osg::MatrixTransform;
            osg::Matrix m;

            shell->generateMesh();

            m.setTrans(osg::Vec3(shell->getTopoShape().Location().Transformation().TranslationPart().X(),
                                 shell->getTopoShape().Location().Transformation().TranslationPart().Y(),
                                 shell->getTopoShape().Location().Transformation().TranslationPart().Z()));
            m.setRotate(osg::Quat(shell->getTopoShape().Location().Transformation().GetRotation().X(),
                        shell->getTopoShape().Location().Transformation().GetRotation().Y(),
                        shell->getTopoShape().Location().Transformation().GetRotation().Z(),
                        shell->getTopoShape().Location().Transformation().GetRotation().W()));
            t_shell->setMatrix(m);

            t_shell->addChild(shell);
            s_shell->addChild(t_shell);
            _shapeTree->addChild(s_shell);
        }
        for(explorer.Init(aShape,TopAbs_FACE,TopAbs_SHELL);explorer.More();explorer.Next())
        {
            TopoDS_Shape s = explorer.Current();
            osg::ref_ptr<OceShape> face = new OceShape(s);
            osg::ref_ptr<osg::Switch> s_face= new osg::Switch;
            osg::ref_ptr<osg::MatrixTransform> t_face= new osg::MatrixTransform;
            osg::Matrix m;

            face->generateMesh();

            m.setTrans(osg::Vec3(face->getTopoShape().Location().Transformation().TranslationPart().X(),
                                 face->getTopoShape().Location().Transformation().TranslationPart().Y(),
                                 face->getTopoShape().Location().Transformation().TranslationPart().Z()));
            m.setRotate(osg::Quat(face->getTopoShape().Location().Transformation().GetRotation().X(),
                        face->getTopoShape().Location().Transformation().GetRotation().Y(),
                        face->getTopoShape().Location().Transformation().GetRotation().Z(),
                        face->getTopoShape().Location().Transformation().GetRotation().W()));
            t_face->setMatrix(m);

            t_face->addChild(face);
            s_face->addChild(t_face);
            _shapeTree->addChild(s_face);
        }
        for(explorer.Init(aShape,TopAbs_WIRE,TopAbs_FACE);explorer.More();explorer.Next())
        {
            TopoDS_Shape s = explorer.Current();
            osg::ref_ptr<OceShape> wire = new OceShape(s);

            osg::ref_ptr<osg::Switch> s_wire= new osg::Switch;
            osg::ref_ptr<osg::MatrixTransform> t_wire= new osg::MatrixTransform;
            osg::Matrix m;

            wire->generateMesh();

            m.setTrans(osg::Vec3(wire->getTopoShape().Location().Transformation().TranslationPart().X(),
                                 wire->getTopoShape().Location().Transformation().TranslationPart().Y(),
                                 wire->getTopoShape().Location().Transformation().TranslationPart().Z()));
            m.setRotate(osg::Quat(wire->getTopoShape().Location().Transformation().GetRotation().X(),
                        wire->getTopoShape().Location().Transformation().GetRotation().Y(),
                        wire->getTopoShape().Location().Transformation().GetRotation().Z(),
                        wire->getTopoShape().Location().Transformation().GetRotation().W()));
            t_wire->setMatrix(m);

            t_wire->addChild(wire);
            s_wire->addChild(t_wire);
            _shapeTree->addChild(s_wire);
        }
        for(explorer.Init(aShape,TopAbs_EDGE,TopAbs_WIRE);explorer.More();explorer.Next())
        {
            TopoDS_Shape s = explorer.Current();
            osg::ref_ptr<OceShape> edge = new OceShape(s);

            osg::ref_ptr<osg::Switch> s_edge= new osg::Switch;
            osg::ref_ptr<osg::MatrixTransform> t_edge= new osg::MatrixTransform;
            osg::Matrix m;

            edge->generateMesh();

            m.setTrans(osg::Vec3(edge->getTopoShape().Location().Transformation().TranslationPart().X(),
                                 edge->getTopoShape().Location().Transformation().TranslationPart().Y(),
                                 edge->getTopoShape().Location().Transformation().TranslationPart().Z()));
            m.setRotate(osg::Quat(edge->getTopoShape().Location().Transformation().GetRotation().X(),
                        edge->getTopoShape().Location().Transformation().GetRotation().Y(),
                        edge->getTopoShape().Location().Transformation().GetRotation().Z(),
                        edge->getTopoShape().Location().Transformation().GetRotation().W()));
            t_edge->setMatrix(m);


            t_edge->addChild(edge);
            s_edge->addChild(t_edge);
            _shapeTree->addChild(s_edge);
        }
        for(explorer.Init(aShape,TopAbs_VERTEX,TopAbs_EDGE);explorer.More();explorer.Next())
        {
            TopoDS_Shape s = explorer.Current();
            osg::ref_ptr<OceShape> vertex = new OceShape(s);
            osg::ref_ptr<osg::Switch> s_vertex= new osg::Switch;
            osg::ref_ptr<osg::MatrixTransform> t_vertex= new osg::MatrixTransform;
            osg::Matrix m;

            vertex->generateMesh();

            m.setTrans(osg::Vec3(vertex->getTopoShape().Location().Transformation().TranslationPart().X(),
                                 vertex->getTopoShape().Location().Transformation().TranslationPart().Y(),
                                 vertex->getTopoShape().Location().Transformation().TranslationPart().Z()));

            t_vertex->setMatrix(m);

            t_vertex->addChild(vertex);
            s_vertex->addChild(t_vertex);
            _shapeTree->addChild(s_vertex);
        }
        }
    }

}

void StepFileRW::writeFile(std::string path)
{
    WriteToStepVisitor visitor(path);
    _shapeTree->accept(visitor);
    visitor.write();
}

void WriteToStepVisitor::write()
{
    int code = _writer.Write(_path.c_str());
    if(code != IFSelect_RetDone)
        std::cout<<"error in writeing to file\n";
}

void WriteToStepVisitor::apply(osg::Geode &t_node)
{
    if(std::string(t_node.className()) == "OceShape")
    {
        if(_writer.Transfer(dynamic_cast<OceShape&>(t_node).getTopoShape(),STEPControl_AsIs) != IFSelect_RetDone)
            std::cout<<"error in transfer to step";
        std::cout<<"entered oceshape\n";
    }
    traverse(t_node);
}

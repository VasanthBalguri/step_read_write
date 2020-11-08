#ifndef OCEFEATURE_H
#define OCEFEATURE_H

#include "oceincludes.h"
#include "osgincludes.h"

class OceShape:public osg::Geode
{
    osg::ref_ptr<osg::Geometry> _geom;
    TopoDS_Shape _shape;
    gp_Trsf _transform;
    osg::Vec3 _color;
public:
    OceShape(TopoDS_Shape shape):_shape(shape)
    {
        _geom = new osg::Geometry();
        _color = osg::Vec3(1.0,1.0,1.0);
    }
    OceShape(osg::Geode& geode)
    {

    }
    TopoDS_Shape getTopoShape()
    {return _shape;}

    void setTopoShape(TopoDS_Shape &shape)
    {
        _shape = shape.EmptyCopied();
        _transform = shape.Location().Transformation();
    }

    void generateMesh();
    TopAbs_ShapeEnum getShapeType(){return _shape.ShapeType();}
    //required to identify class, especially for node traversal operations
    virtual const char* className() const {return "OceShape";}
};

/* feature tree:
*   - top level trasform
*   - next level switch
*   - next level transform
*   - next level geode -> contains OceShape
*
* Note: this is temporary for now
* */
class StepFileRW
{
    STEPControl_Reader _aReader;
    gp_Trsf _transform;
    osg::ref_ptr<osg::MatrixTransform> _shapeTree;
    osg::ref_ptr<OceShape> _shape;

    //yet to be implemented
    //void getNameandColor();
public:
    void readFile(std::string path);
    void writeFile(std::string path);
    //yet to be implemented
//    int shapeCount();
//    void reportShapes();
    osg::Transform* getShapeTree(){return _shapeTree.get();}

};

class WriteToStepVisitor:public osg::NodeVisitor
{
    STEPControl_Writer _writer;
    std::string _path;
public:
    WriteToStepVisitor(std::string path):_path(path)
    {
        setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
    }

    void write();
    virtual void apply(osg::Node &t_node)
    {
        traverse(t_node);
    }

    virtual void apply(osg::Geode &t_node);

};

#endif // OCEFEATURE_H

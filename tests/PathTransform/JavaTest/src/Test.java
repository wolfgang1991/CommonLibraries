import de.vhf.pathtransform.*;

public class Test{

	public static void main(String[] args) {
		PathTransform pt = new PathTransform("TriangulatedPathTransfrom,2,10,4,457,528,1105,825,460,398,929,557,920,528,846,754,984,415,1445,601,6,2,1,3,1,2,0");
		pt.print();
		PathTransform.Path path = new PathTransform.Path(2);
		path.pushBack(new PathTransform.Path.Point(100,100,100));
		path.pushBack(new PathTransform.Path.Point(1000,1000,1000));
		pt.transformInPlace(path);
		System.out.println("path:");
		path.print();
		PathTransform.Path path2 = new PathTransform.Path(2);
		path2.pushBack(new PathTransform.Path.Point(100,100,100));
		path2.pushBack(new PathTransform.Path.Point(1000,1000,1000));
		PathTransform.Path path3 = pt.transform(path2);
		System.out.println("path3:");
		path3.print();
		PathTransform.Path.Point testPoint = new PathTransform.Path.Point(1,1,0);
		PathTransform.Path.Point result = pt.transformSingle(testPoint);
		System.out.println("result: x: "+result.x+" y: "+result.y+" z: "+result.z);
		path.dispose();
		path2.dispose();
		path3.dispose();
		pt.dispose();
	}
	
};

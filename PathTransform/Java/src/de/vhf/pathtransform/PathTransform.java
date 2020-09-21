package de.vhf.pathtransform;

public class PathTransform {
	static {
		// Load native library PathTransform.dll (Windows) or libPathTransform.so (Unices)
		System.loadLibrary("PathTransform");
	}

	public static class Path {
		public static class Point {
			public long x;
			public long y;
			public long z;

			public Point(long x, long y, long z) {
				this.x = x;
				this.y = y;
				this.z = z;
			}
		}

		private long ptr = 0;

		private static native long createPath(int reservedSpace);

		private static native void deletePath(long pathPtr);

		private static native void pushBack(long pathPtr, long x, long y, long z);

		private static native long getPointPtr(long pathPtr, int index);

		private static native long getPointX(long pointPtr);

		private static native long getPointY(long pointPtr);

		private static native long getPointZ(long pointPtr);
		
		private static native void deletePoint(long pointPtr);

		private static native int getPointCount(long pathPtr);

		public Path(int reservedSpace) {
			ptr = createPath(reservedSpace);
		}

		public Path(long pointer) {
			this.ptr = pointer;
		}

		public void dispose() {
			checkPointer(ptr);
			deletePath(ptr);
			ptr = 0;
		}

		public void pushBack(Point p) {
			checkPointer(ptr);
			pushBack(ptr, p.x, p.y, p.z);
		}

		public Point getPoint(int index) {
			checkPointer(ptr);
			long pptr = getPointPtr(ptr, index);
			return new Point(getPointX(pptr), getPointY(pptr), getPointZ(pptr));
		}

		public int getPointCount() {
			checkPointer(ptr);
			return getPointCount(ptr);
		}

		public void print() {
			int pcnt = getPointCount();
			for (int i = 0; i < pcnt; i++) {
				Point p = getPoint(i);
				System.out.println("x: " + p.x + " y: " + p.y + " z: " + p.z);
			}
		}
		
	}

	private long ptr = 0;

	private static native long createPathTransform(String representation);

	private static native void printPathTransform(long ptr);

	private static native void deletePathTransform(long ptr);

	private static native void transformInPlace(long ptr, long pathPtr);

	private static native long transform(long ptr, long pathPtr);
	
	private static native long transformSingle(long ptr, long x, long y, long z);

	private static void checkPointer(long p) {
		if (p == 0) {
			throw new IllegalStateException("Internal Native Null Pointer detected");
		}
	}

	public PathTransform(String representation) {
		ptr = createPathTransform(representation);
		if (ptr == 0) {
			throw new IllegalArgumentException("Invalid String Representation of Transformation");
		}
	}

	public void print() {
		checkPointer(ptr);
		printPathTransform(ptr);
	}

	public void dispose() {
		checkPointer(ptr);
		deletePathTransform(ptr);
		ptr = 0;
	}
	
	public Path.Point transformSingle(Path.Point p){
		checkPointer(ptr);
		long pointPtr = transformSingle(ptr, p.x, p.y, p.z);
		Path.Point result = new Path.Point(Path.getPointX(pointPtr), Path.getPointY(pointPtr), Path.getPointZ(pointPtr));
		Path.deletePoint(pointPtr);
		return result;
	}

	/** transforms a path without subdivisions by modifying its coordinates directly */
	public void transformInPlace(Path path) {
		checkPointer(ptr);
		checkPointer(path.ptr);
		transformInPlace(ptr, path.ptr);
	}

	/**
	 * creates a new path with the transformed input path as content. This also enables subdivisions if applicable. (=>
	 * The resulting path may have more points than the input path.)
	 */
	public Path transform(Path path) {
		long pathPtr;

		checkPointer(ptr);
		checkPointer(path.ptr);
		pathPtr = transform(ptr, path.ptr);
		return new Path(pathPtr);
	}
}

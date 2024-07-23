#include <gesture/gesture.h>
#include <gesture/gesturestroke.h>
namespace cdroid{
Gesture::Gesture() {
    mGestureID = GESTURE_ID_BASE + sGestureCount++;
}

Gesture* Gesture::clone() {
    Gesture* gesture = new Gesture();
    gesture->mBoundingBox.set(mBoundingBox.left, mBoundingBox.top,
                                    mBoundingBox.width, mBoundingBox.height);
    const int count = mStrokes.size();
    for (int i = 0; i < count; i++) {
        GestureStroke* stroke = mStrokes.at(i);
        gesture->mStrokes.push_back((GestureStroke*)stroke->clone());
    }
    return gesture;
}

/**
 * @return all the strokes of the gesture
 */
std::vector<GestureStroke*> Gesture::getStrokes() {
    return mStrokes;
}

/**
 * @return the number of strokes included by this gesture
 */
int Gesture::getStrokesCount()const {
    return mStrokes.size();
}

/**
 * Adds a stroke to the gesture.
 *
 * @param stroke
 */
void Gesture::addStroke(GestureStroke* stroke) {
    mStrokes.push_back(stroke);
    mBoundingBox.Union(stroke->boundingBox);
}

/**
 * Calculates the total length of the gesture. When there are multiple strokes in
 * the gesture, this returns the sum of the lengths of all the strokes.
 *
 * @return the length of the gesture
 */
float Gesture::getLength() const{
    int len = 0;
    const std::vector<GestureStroke*>& strokes = mStrokes;
    const int count = strokes.size();
    for (int i = 0; i < count; i++) {
        len += strokes.at(i)->length;
    }
    return len;
}

/**
 * @return the bounding box of the gesture
 */
RectF Gesture::getBoundingBox()const {
    return mBoundingBox;
}

cdroid::Path Gesture::toPath() {
    return toPath(nullptr);
}

cdroid::Path Gesture::toPath(Path* path) {
    if (path == nullptr) path = new Path();

    const std::vector<GestureStroke*> strokes = mStrokes;
    const int count = strokes.size();

    for (int i = 0; i < count; i++) {
        path->addPath(strokes.at(i)->getPath());
    }

    return path;
}

cdroid::Path Gesture::toPath(int width, int height, int edge, int numSample) {
    return toPath(nullptr, width, height, edge, numSample);
}

cdroid::Path Gesture::toPath(Path* path, int width, int height, int edge, int numSample) {
    if (path == nullptr) path = new cdroid::Path();

    const std::vector<GestureStroke*>& strokes = mStrokes;
    const int count = strokes.size();

    for (int i = 0; i < count; i++) {
        path->addPath(strokes.at(i)->toPath(width - 2 * edge, height - 2 * edge, numSample));
    }

    return path;
}

/**
 * Sets the id of the gesture.
 *
 * @param id
 */
void Gesture::setID(long id) {
    mGestureID = id;
}

/**
 * @return the id of the gesture
 */
long Gesture::getID() const{
    return mGestureID;
}

/**
 * Creates a bitmap of the gesture with a transparent background.
 *
 * @param width width of the target bitmap
 * @param height height of the target bitmap
 * @param edge the edge
 * @param numSample
 * @param color
 * @return the bitmap
 */
Bitmap Gesture::toBitmap(int width, int height, int edge, int numSample, int color) {
    Bitmap bitmap = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32,width, height);
    Canvas canvas(bitmap);

    canvas.translate(edge, edge);

    canvas.set_color(color);

    for (auto stroke:mStrokes) {
        Path path = stroke->toPath(width - 2 * edge, height - 2 * edge, numSample);
        canvas.drawPath(path);
    }

    return bitmap;
}

/**
 * Creates a bitmap of the gesture with a transparent background.
 *
 * @param width
 * @param height
 * @param inset
 * @param color
 * @return the bitmap
 */
Bitmap Gesture::toBitmap(int width, int height, int inset, int color) {
    Bitmap bitmap = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32,width, height);
    Canvas canvas(bitmap);

    canvas.set_color(color);

    Path path = toPath();
    RectF bounds;
    path.compute_bounds(bounds, true);

    const float sx = (width - 2 * inset) / bounds.width;
    const float sy = (height - 2 * inset) / bounds.height;
    const float scale = sx > sy ? sy : sx;
    canvas.set_line_width(2.0f / scale);

    //path.offset(-bounds.left + (width - bounds.width * scale) / 2.0f,
    //        -bounds.top + (height - bounds.height * scale) / 2.0f);
    float offsetX =-bounds.left + (width - bounds.width * scale) / 2.0f;
    float offsetY =-bounds.top + (height - bounds.height * scale) / 2.0f;

    canvas.translate(offsetX + inset, offsetY + inset);
    canvas.scale(scale, scale);

    canvas.drawPath(path);

    return bitmap;
}

void Gesture::serialize(std::ostream& out){
    const std::vector<GestureStroke*>& strokes = mStrokes;
    const int count = strokes.size();

    // Write gesture ID
    //out.writeLong(mGestureID);
    // Write number of strokes
    //out.writeInt(count);

    for (int i = 0; i < count; i++) {
        strokes.at(i)->serialize(out);
    }
}

Gesture Gesture::deserialize(std::istream& in){
    Gesture gesture;

    // Gesture ID
    //gesture.mGestureID = in.readLong();
    // Number of strokes
    int count;// = in.readInt();

    for (int i = 0; i < count; i++) {
        gesture.addStroke(GestureStroke::deserialize(in));
    }

    return gesture;
}
#if 0
public static final @android.annotation.NonNull Parcelable.Creator<Gesture> CREATOR = new Parcelable.Creator<Gesture>() {
    public Gesture createFromParcel(Parcel in) {
        Gesture gesture = null;
        final long gestureID = in.readLong();

        final DataInputStream inStream = new DataInputStream(
                new ByteArrayInputStream(in.createByteArray()));

        try {
            gesture = deserialize(inStream);
        } catch (IOException e) {
            Log.e(GestureConstants.LOG_TAG, "Error reading Gesture from parcel:", e);
        } finally {
            GestureUtils.closeStream(inStream);
        }

        if (gesture != null) {
            gesture.mGestureID = gestureID;
        }

        return gesture;
    }

    public Gesture[] newArray(int size) {
        return new Gesture[size];
    }
};

public void writeToParcel(Parcel out, int flags) {
    out.writeLong(mGestureID);

    boolean result = false;
    final ByteArrayOutputStream byteStream =
            new ByteArrayOutputStream(GestureConstants.IO_BUFFER_SIZE);
    final DataOutputStream outStream = new DataOutputStream(byteStream);

    try {
        serialize(outStream);
        result = true;
    } catch (IOException e) {
        Log.e(GestureConstants.LOG_TAG, "Error writing Gesture to parcel:", e);
    } finally {
        GestureUtils.closeStream(outStream);
        GestureUtils.closeStream(byteStream);
    }

    if (result) {
        out.writeByteArray(byteStream.toByteArray());
    }
}

public int describeContents() {
    return 0;
}
#endif
}

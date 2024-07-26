#include <gesture/instance.h>
#include <gesture/gesture.h>
#include <gesture/gestureutils.h>
#include <gesture/gesturestore.h>
namespace cdroid{

Instance::Instance(long id,const std::vector<float>& sample, const std::string& sampleName)
    :id(id),vector(sample),label(sampleName){
}

void Instance::normalize() {
    std::vector<float>& sample = vector;
    float sum = 0;

    const int size = sample.size();
    for (int i = 0; i < size; i++) {
        sum += sample[i] * sample[i];
    }

    const float magnitude = (float)std::sqrt(sum);
    for (int i = 0; i < size; i++) {
        sample[i] /= magnitude;
    }
}

/**
 * create a learning instance for a single stroke gesture
 *
 * @param gesture
 * @param label
 * @return the instance
 */
Instance* Instance::createInstance(int sequenceType, int orientationType,const Gesture& gesture, const std::string& label) {
    std::vector<float> pts;
    Instance* instance;
    if (sequenceType == GestureStore::SEQUENCE_SENSITIVE) {
        pts = temporalSampler(orientationType, gesture);
        instance = new Instance(gesture.getID(), pts, label);
        instance->normalize();
    } else {
        pts = spatialSampler(gesture);
        instance = new Instance(gesture.getID(), pts, label);
    }
    return instance;
}

std::vector<float> Instance::spatialSampler(const Gesture& gesture) {
    return GestureUtils::spatialSampling(gesture, PATCH_SAMPLE_SIZE, false);
}

static float ORIENTATIONS[] = {
     0, (float) (M_PI / 4), (float) (M_PI / 2),
     (float) (M_PI * 3 / 4),(float) M_PI, -0,
     (float) (-M_PI / 4), (float) (-M_PI / 2),
     (float) (-M_PI * 3 / 4), (float) -M_PI
};

std::vector<float> Instance::temporalSampler(int orientationType,const Gesture& gesture) {
    std::vector<float> pts = GestureUtils::temporalSampling(*gesture.getStrokes().at(0),
            SEQUENCE_SAMPLE_SIZE);
    std::vector<float> center = GestureUtils::computeCentroid(pts);
    float orientation = (float)std::atan2(pts[1] - center[1], pts[0] - center[0]);

    float adjustment = -orientation;
    if (orientationType != GestureStore::ORIENTATION_INVARIANT) {
        const int count = sizeof(ORIENTATIONS)/sizeof(float);
        for (int i = 0; i < count; i++) {
            float delta = ORIENTATIONS[i] - orientation;
            if (std::abs(delta) < std::abs(adjustment)) {
                adjustment = delta;
            }
        }
    }

    GestureUtils::translate(pts, -center[0], -center[1]);
    GestureUtils::rotate(pts, adjustment);

    return pts;
}

}

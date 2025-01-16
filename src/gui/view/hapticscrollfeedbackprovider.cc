#include <view/view.h>
#include <view/hapticfeedbackconstants.h>
#include <view/hapticscrollfeedbackprovider.h>
#include <view/hapticfeedbackconstants.h>
namespace cdroid{

ScrollFeedbackProvider* ScrollFeedbackProvider::createProvider(View* view) {
    return new HapticScrollFeedbackProvider(view);
}

HapticScrollFeedbackProvider::HapticScrollFeedbackProvider(View* view)
    :HapticScrollFeedbackProvider(view, &ViewConfiguration::get(view->getContext()),
            /* disabledIfViewPlaysScrollHaptics= */ true){
}

HapticScrollFeedbackProvider::HapticScrollFeedbackProvider(
        View* view, ViewConfiguration* viewConfig, bool disabledIfViewPlaysScrollHaptics) {
    mView = view;
    mViewConfig = viewConfig;
    mDisabledIfViewPlaysScrollHaptics = disabledIfViewPlaysScrollHaptics;
}

void HapticScrollFeedbackProvider::onScrollProgress(int inputDeviceId, int source, int axis, int deltaInPixels) {
    maybeUpdateCurrentConfig(inputDeviceId, source, axis);
    if (!mHapticScrollFeedbackEnabled) {
        return;
    }

    // Unlock limit feedback regardless of scroll tick being enabled as long as there's a
    // non-zero scroll progress.
    if (deltaInPixels != 0) {
        mCanPlayLimitFeedback = true;
    }

    if (mTickIntervalPixels == TICK_INTERVAL_NO_TICK) {
        // There's no valid tick interval. Exit early before doing any further computation.
        return;
    }

    mTotalScrollPixels += deltaInPixels;

    if (std::abs(mTotalScrollPixels) >= mTickIntervalPixels) {
        mTotalScrollPixels %= mTickIntervalPixels;
        /*if (android.os.vibrator.Flags.hapticFeedbackInputSourceCustomizationEnabled()) {
            mView->performHapticFeedbackForInputDevice(
                    HapticFeedbackConstants::SCROLL_TICK, inputDeviceId, source, 0);
        } else */{
            mView->performHapticFeedback(HapticFeedbackConstants::SCROLL_TICK);
        }
    }
}

void HapticScrollFeedbackProvider::onScrollLimit(int inputDeviceId, int source, int axis, bool isStart) {
    maybeUpdateCurrentConfig(inputDeviceId, source, axis);
    if (!mHapticScrollFeedbackEnabled) {
        return;
    }

    if (!mCanPlayLimitFeedback) {
        return;
    }
    /*if (android.os.vibrator.Flags.hapticFeedbackInputSourceCustomizationEnabled()) {
        mView->performHapticFeedbackForInputDevice(
                HapticFeedbackConstants::SCROLL_LIMIT, inputDeviceId, source, 0);
    } else */{
        mView->performHapticFeedback(HapticFeedbackConstants::SCROLL_LIMIT);
    }

    mCanPlayLimitFeedback = false;
}

void HapticScrollFeedbackProvider::onSnapToItem(int inputDeviceId, int source, int axis) {
    maybeUpdateCurrentConfig(inputDeviceId, source, axis);
    if (!mHapticScrollFeedbackEnabled) {
        return;
    }
    /*if (android.os.vibrator.Flags.hapticFeedbackInputSourceCustomizationEnabled()) {
        mView->performHapticFeedbackForInputDevice(
                HapticFeedbackConstants::SCROLL_ITEM_FOCUS, inputDeviceId, source,0);
    } else */{
        mView->performHapticFeedback(HapticFeedbackConstants::SCROLL_ITEM_FOCUS);
    }
    mCanPlayLimitFeedback = true;
}

void HapticScrollFeedbackProvider::maybeUpdateCurrentConfig(int deviceId, int source, int axis) {
    if (mAxis != axis || mSource != source || mDeviceId != deviceId) {
        mSource = source;
        mAxis = axis;
        mDeviceId = deviceId;

        if (mDisabledIfViewPlaysScrollHaptics  && (source == InputDevice::SOURCE_ROTARY_ENCODER)
                && mViewConfig->isViewBasedRotaryEncoderHapticScrollFeedbackEnabled()) {
            mHapticScrollFeedbackEnabled = false;
            return;
        }

        mHapticScrollFeedbackEnabled =  mViewConfig->isHapticScrollFeedbackEnabled(deviceId, axis, source);
        mCanPlayLimitFeedback = INITIAL_END_OF_LIST_HAPTICS_ENABLED;
        mTotalScrollPixels = 0;
        updateTickIntervals(deviceId, source, axis);
    }
}

void HapticScrollFeedbackProvider::updateTickIntervals(int deviceId, int source, int axis) {
    mTickIntervalPixels = mHapticScrollFeedbackEnabled
            ? mViewConfig->getHapticScrollFeedbackTickInterval(deviceId, axis, source)
            : TICK_INTERVAL_NO_TICK;
}
}

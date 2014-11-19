/*
 * Copyright (C) 2012 STMicroelectronics
 * Giuseppe Barba, Alberto Marinoni - Motion MEMS Product Div.
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "configuration.h"
#if (SENSORS_STEP_DETECTOR_ENABLE == 1)

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <string.h>
#include "StepDetectorSensor.h"


/*****************************************************************************/
int StepDetectorSensor::mEnabled = 0;

StepDetectorSensor::StepDetectorSensor()
	: SensorBase(NULL, SENSOR_DATANAME_STEP_D),
	mInputReader(4)
{
	mPendingEvent.version = sizeof(sensors_event_t);
	mPendingEvent.sensor = ID_STEP_DETECTOR;
	mPendingEvent.type = SENSOR_TYPE_STEP_DETECTOR;
	mPendingEvent.data[0] = 1.0f;

	if (data_fd) {
		STLOGI("StepDetectorSensor::StepDetectorSensor device_sysfs_path:(%s)", sysfs_device_path);
	} else {
		STLOGE("StepDetectorSensor::StepDetectorSensor device_sysfs_path:(%s) not found", sysfs_device_path);
	}
}

StepDetectorSensor::~StepDetectorSensor()
{
	if (mEnabled) {
		enable(SENSORS_STEP_DETECTOR_HANDLE, 0, 0);
	}
}

int StepDetectorSensor::enable(int32_t handle, int en, int  __attribute__((unused))type)
{
	int err = 0;
	int flags = en ? 1 : 0;
	int mEnabledPrev;

	if (flags) {
		if (!mEnabled) {
			err = writeEnable(SENSORS_STEP_DETECTOR_HANDLE, flags);
			if(err >= 0)
				err = 0;
		}
		mEnabled = 1;
	} else {
		if (mEnabled){
			err = writeEnable(SENSORS_STEP_DETECTOR_HANDLE, flags);
			if(err >= 0) {
				err = 0;
				mEnabled = 0;
			}
		}
	}

	if(err >= 0 ) {
		STLOGD("StepDetectorSensor::enable(%d), handle: %d, mEnabled: %d",
						flags, handle, mEnabled);
	} else {
		STLOGE("StepDetectorSensor::enable(%d), handle: %d, mEnabled: %d",
						flags, handle, mEnabled);
	}

	return err;
}

int StepDetectorSensor::getWhatFromHandle(int32_t __attribute__((unused))handle)
{
	return 0;
}

bool StepDetectorSensor::hasPendingEvents() const
{
	return false;
}

int StepDetectorSensor::setDelay(int32_t __attribute__((unused))handle,
					int64_t __attribute__((unused))delay_ns)
{
	return 0;
}

int StepDetectorSensor::readEvents(sensors_event_t* data, int count)
{
	if (count < 1)
		return -EINVAL;

	ssize_t n = mInputReader.fill(data_fd);
	if (n < 0)
		return n;

	int numEventReceived = 0;
	input_event const* event;

	while (count && mInputReader.readEvent(&event)) {

		if (event->type == EVENT_TYPE_STEP_D) {

#if (DEBUG_STEP_D == 1)
	STLOGD("StepDetectorSensor::readEvents (event_code=%d)", event->code);
#endif
			switch(event->code) {
			case EVENT_TYPE_STEP_D_DATA:

				break;
#if defined(INPUT_EVENT_HAS_TIMESTAMP)
			case EVENT_TYPE_TIME_MSB:
				timestamp = ((int64_t)(event->value)) << 32;

				break;
			case EVENT_TYPE_TIME_LSB:
				timestamp |= (uint32_t)(event->value);

				break;
#endif
			default:
				STLOGE("StepDetectorSensor: unknown event code \
					(type = %d, code = %d)", event->type,
								event->code);
			}
		} else if (event->type == EV_SYN) {

#if !defined(INPUT_EVENT_HAS_TIMESTAMP)
			timestamp = timevalToNano(event->time);
#endif
			mPendingEvent.timestamp = timestamp;
			memcpy(data, &mPendingEvent, sizeof(mPendingEvent));
			data++;
			count--;
			numEventReceived++;
#if (DEBUG_STEP_D == 1)
			STLOGD("StepDetectorSensor::readEvents (time = %lld), count(%d), received(%d)",
						mPendingEvent.timestamp,
						count, numEventReceived);
#endif

		} else {
			STLOGE("StepDetectorSensor: unknown event (type=%d, code=%d)",
						event->type, event->code);
		}
no_data:
		mInputReader.next();
	}

	return numEventReceived;
}

#endif /* SENSORS_STEP_DETECTOR_ENABLE */

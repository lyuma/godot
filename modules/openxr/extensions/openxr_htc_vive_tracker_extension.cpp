/**************************************************************************/
/*  openxr_htc_vive_tracker_extension.cpp                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "openxr_htc_vive_tracker_extension.h"

#include "../action_map/openxr_interaction_profile_metadata.h"
#include "../openxr_api.h"

#include "core/string/print_string.h"

HashMap<String, bool *> OpenXRHTCViveTrackerExtension::get_requested_extensions() {
	HashMap<String, bool *> request_extensions;

	request_extensions[XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME] = &available;

	return request_extensions;
}

PackedStringArray OpenXRHTCViveTrackerExtension::get_suggested_tracker_names() {
	PackedStringArray arr = {
		"/user/vive_tracker_htcx/role/handheld_object",
		"/user/vive_tracker_htcx/role/left_foot",
		"/user/vive_tracker_htcx/role/right_foot",
		"/user/vive_tracker_htcx/role/left_shoulder",
		"/user/vive_tracker_htcx/role/right_shoulder",
		"/user/vive_tracker_htcx/role/left_elbow",
		"/user/vive_tracker_htcx/role/right_elbow",
		"/user/vive_tracker_htcx/role/left_knee",
		"/user/vive_tracker_htcx/role/right_knee",
		"/user/vive_tracker_htcx/role/waist",
		"/user/vive_tracker_htcx/role/chest",
		"/user/vive_tracker_htcx/role/camera",
		"/user/vive_tracker_htcx/role/keyboard",
		"/user/vive_tracker_htcx/role/left_wrist",
		"/user/vive_tracker_htcx/role/right_wrist",
		"/user/vive_tracker_htcx/role/left_ankle",
		"/user/vive_tracker_htcx/role/right_ankle",
	};
	return arr;
}

bool OpenXRHTCViveTrackerExtension::is_available() {
	return available;
}

void OpenXRHTCViveTrackerExtension::poll_tracker_list() {
	OpenXRAPI *openxr_api = OpenXRAPI::get_singleton();
	uint32_t tracker_count = 0;
	XrResult result = xrEnumerateViveTrackerPathsHTCX(
			openxr_api->get_instance(),
			0, // pathCapacityInput = 0 (just asking for count)
			&tracker_count,
			nullptr // paths = NULL
	);
	print_line("    == POLL: ", (int)tracker_count, " Vive Trackers: ", (int)result);

	if (result == XR_SUCCESS && tracker_count > 0) {
		// 2. Allocate memory for the results
		LocalVector<XrViveTrackerPathsHTCX> trackers;
		trackers.resize(tracker_count);

		// 3. Second call: Get the actual data
		result = xrEnumerateViveTrackerPathsHTCX(
				openxr_api->get_instance(),
				tracker_count, // pathCapacityInput = actual size
				&tracker_count,
				&(trackers[0]));
		for (const XrViveTrackerPathsHTCX &paths : trackers) {
			String persistentPath = openxr_api->get_xr_path_name(paths.persistentPath);
			if (paths.rolePath != XR_NULL_PATH) {
				String rolePath = openxr_api->get_xr_path_name(paths.rolePath);
				print_line("      -> POLL: Vive Tracker currently at role ", rolePath, ": ", persistentPath);
			} else {
				print_line("      -> POLL: Vive Tracker currently at XR_NULL_PATH: ", persistentPath);
				OpenXRInteractionProfileMetadata *openxr_metadata = OpenXRInteractionProfileMetadata::get_singleton();
				openxr_metadata->register_top_level_path(persistentPath, persistentPath, XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
				RID new_interaction_profile = openxr_api->interaction_profile_create(persistentPath);
			}

		}
	}
}

void OpenXRHTCViveTrackerExtension::on_instance_created(const XrInstance p_instance) {
	EXT_INIT_XR_FUNC(xrEnumerateViveTrackerPathsHTCX);
}

void OpenXRHTCViveTrackerExtension::on_register_metadata() {
	OpenXRInteractionProfileMetadata *openxr_metadata = OpenXRInteractionProfileMetadata::get_singleton();
	ERR_FAIL_NULL(openxr_metadata);

	// register_top_level_path("Handheld object tracker", "/user/vive_tracker_htcx/role/handheld_object", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Left foot tracker", "/user/vive_tracker_htcx/role/left_foot", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Right foot tracker", "/user/vive_tracker_htcx/role/right_foot", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Left shoulder tracker", "/user/vive_tracker_htcx/role/left_shoulder", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Right shoulder tracker", "/user/vive_tracker_htcx/role/right_shoulder", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Left elbow tracker", "/user/vive_tracker_htcx/role/left_elbow", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Right elbow tracker", "/user/vive_tracker_htcx/role/right_elbow", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Left knee tracker", "/user/vive_tracker_htcx/role/left_knee", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Right knee tracker", "/user/vive_tracker_htcx/role/right_knee", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Waist tracker", "/user/vive_tracker_htcx/role/waist", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Chest tracker", "/user/vive_tracker_htcx/role/chest", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Camera tracker", "/user/vive_tracker_htcx/role/camera", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Keyboard tracker", "/user/vive_tracker_htcx/role/keyboard", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Left wrist tracker", "/user/vive_tracker_htcx/role/left_wrist", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Right wrist tracker", "/user/vive_tracker_htcx/role/right_wrist", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Left ankle tracker", "/user/vive_tracker_htcx/role/left_ankle", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
	openxr_metadata->register_top_level_path("Right ankle tracker", "/user/vive_tracker_htcx/role/right_ankle", XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);

	{ // HTC Vive tracker
		// Interestingly enough trackers don't have buttons or inputs, yet these are defined in the spec.
		// I think this can be supported through attachments on the trackers.
		const String profile_path = "/interaction_profiles/htc/vive_tracker_htcx";
		openxr_metadata->register_interaction_profile("HTC Vive tracker", profile_path, XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
		for (const String user_path : {
					 /* "/user/vive_tracker_htcx/role/handheld_object", */
					 "/user/vive_tracker_htcx/role/left_foot",
					 "/user/vive_tracker_htcx/role/right_foot",
					 "/user/vive_tracker_htcx/role/left_shoulder",
					 "/user/vive_tracker_htcx/role/right_shoulder",
					 "/user/vive_tracker_htcx/role/left_elbow",
					 "/user/vive_tracker_htcx/role/right_elbow",
					 "/user/vive_tracker_htcx/role/left_knee",
					 "/user/vive_tracker_htcx/role/right_knee",
					 "/user/vive_tracker_htcx/role/waist",
					 "/user/vive_tracker_htcx/role/chest",
					 "/user/vive_tracker_htcx/role/camera",
					 "/user/vive_tracker_htcx/role/keyboard",
					 "/user/vive_tracker_htcx/role/left_wrist",
					 "/user/vive_tracker_htcx/role/right_wrist",
					 "/user/vive_tracker_htcx/role/left_ankle",
					 "/user/vive_tracker_htcx/role/right_ankle",
			 }) {
			openxr_metadata->register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose", "", OpenXRAction::OPENXR_ACTION_POSE);

			openxr_metadata->register_io_path(profile_path, "Menu click", user_path, user_path + "/input/menu/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			openxr_metadata->register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value", "", OpenXRAction::OPENXR_ACTION_FLOAT);
			openxr_metadata->register_io_path(profile_path, "Trigger click", user_path, user_path + "/input/trigger/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			openxr_metadata->register_io_path(profile_path, "Squeeze click", user_path, user_path + "/input/squeeze/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			openxr_metadata->register_io_path(profile_path, "Trackpad", user_path, user_path + "/input/trackpad", "", OpenXRAction::OPENXR_ACTION_VECTOR2);
			openxr_metadata->register_io_path(profile_path, "Trackpad click", user_path, user_path + "/input/trackpad/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			openxr_metadata->register_io_path(profile_path, "Trackpad touch", user_path, user_path + "/input/trackpad/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			openxr_metadata->register_io_path(profile_path, "Trackpad Dpad Up", user_path, user_path + "/input/trackpad/dpad_up", "XR_EXT_dpad_binding", OpenXRAction::OPENXR_ACTION_BOOL);
			openxr_metadata->register_io_path(profile_path, "Trackpad Dpad Down", user_path, user_path + "/input/trackpad/dpad_down", "XR_EXT_dpad_binding", OpenXRAction::OPENXR_ACTION_BOOL);
			openxr_metadata->register_io_path(profile_path, "Trackpad Dpad Left", user_path, user_path + "/input/trackpad/dpad_left", "XR_EXT_dpad_binding", OpenXRAction::OPENXR_ACTION_BOOL);
			openxr_metadata->register_io_path(profile_path, "Trackpad Dpad Right", user_path, user_path + "/input/trackpad/dpad_right", "XR_EXT_dpad_binding", OpenXRAction::OPENXR_ACTION_BOOL);
			openxr_metadata->register_io_path(profile_path, "Trackpad Dpad Center", user_path, user_path + "/input/trackpad/dpad_center", "XR_EXT_dpad_binding", OpenXRAction::OPENXR_ACTION_BOOL);

			openxr_metadata->register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic", "", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}
	poll_tracker_list();
}

bool OpenXRHTCViveTrackerExtension::on_event_polled(const XrEventDataBuffer &event) {
	switch (event.type) {
		case XR_TYPE_EVENT_DATA_VIVE_TRACKER_CONNECTED_HTCX: {
			// Investigate if we need to do more here
			OpenXRAPI *openxr_api = OpenXRAPI::get_singleton();
			print_verbose("OpenXR EVENT: VIVE tracker connected");
			const XrEventDataViveTrackerConnectedHTCX &tracker_event =
					reinterpret_cast<const XrEventDataViveTrackerConnectedHTCX &>(event);
			ERR_FAIL_NULL_V(tracker_event.paths, true);
			String persistentPath = openxr_api->get_xr_path_name(tracker_event.paths->persistentPath);
			if (tracker_event.paths->rolePath != XR_NULL_PATH) {
				String rolePath = openxr_api->get_xr_path_name(tracker_event.paths->rolePath);
				print_line("Vive Tracker connected to role ", rolePath, ": ", persistentPath);
			} else {
				print_line("Vive Tracker connected to XR_NULL_PATH: ", persistentPath);
				OpenXRInteractionProfileMetadata *openxr_metadata = OpenXRInteractionProfileMetadata::get_singleton();
				openxr_metadata->register_top_level_path(persistentPath, persistentPath, XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
				RID new_interaction_profile = openxr_api->interaction_profile_create(persistentPath);
			}
			poll_tracker_list();
            return true; // Event handled
		} break;
		case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
			XrSession session = ((const XrEventDataInteractionProfileChanged &)event).session;
			print_line("XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED");
			poll_tracker_list();
			return false;
		} break;
		default: {
			return false;
		} break;
	}
}

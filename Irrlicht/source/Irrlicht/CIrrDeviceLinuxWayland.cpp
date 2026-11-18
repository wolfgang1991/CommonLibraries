// Wayland implementation of CIrrDeviceLinux (used instead of the X11 version if _IRR_LINUX_WAYLAND_ is defined).
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CIrrDeviceLinux.h"

#ifdef _IRR_COMPILE_WITH_WAYLAND_DEVICE_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/utsname.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>

#include <libdecor.h>

#include "IEventReceiver.h"
#include "ISceneManager.h"
#include "IGUIEnvironment.h"
#include "os.h"
#include "CTimer.h"
#include "irrString.h"
#include "Keycodes.h"
#include "COSOperator.h"
#include "CColorConverter.h"
#include "CVideoModeList.h"
#include "SIrrCreationParameters.h"
#include "SExposedVideoData.h"
#include "IGUISpriteBank.h"

#if defined(_IRR_COMPILE_WITH_OPENGL_) || defined(_IRR_COMPILE_WITH_OGLES1_) || defined(_IRR_COMPILE_WITH_OGLES2_) || defined(_IRR_COMPILE_WITH_WEBGL1_)
#include "CEGLManager.h"
#endif

#if defined _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
#include <sys/ioctl.h>
// linux/joystick.h includes linux/input.h, which #defines values for various KEY_FOO keys.
// These override the irr::KEY_FOO equivalents, which stops key handling from working.
// Defining _INPUT_H stops linux/input.h from being included.
#define _INPUT_H
#include <linux/joystick.h>
#undef _INPUT_H
#endif // _IRR_COMPILE_WITH_JOYSTICK_EVENTS_

namespace irr
{
	namespace video
	{
#ifdef _IRR_COMPILE_WITH_OPENGL_
		IVideoDriver* createOpenGLDriver(const irr::SIrrlichtCreationParameters& params, io::IFileSystem* io, IContextManager* contextManager);
#endif

#ifdef _IRR_COMPILE_WITH_OGLES1_
		IVideoDriver* createOGLES1Driver(const irr::SIrrlichtCreationParameters& params, io::IFileSystem* io, IContextManager* contextManager);
#endif

#ifdef _IRR_COMPILE_WITH_OGLES2_
		IVideoDriver* createOGLES2Driver(const irr::SIrrlichtCreationParameters& params, io::IFileSystem* io, IContextManager* contextManager);
#endif

#ifdef _IRR_COMPILE_WITH_WEBGL1_
		IVideoDriver* createWebGL1Driver(const irr::SIrrlichtCreationParameters& params, io::IFileSystem* io, IContextManager* contextManager);
#endif
	}
} // end namespace irr

namespace irr
{

namespace
{
	// linux/input-event-codes.h can't be included (it defines KEY_* macros)
	const u32 WL_BTN_LEFT = 0x110;
	const u32 WL_BTN_RIGHT = 0x111;
	const u32 WL_BTN_MIDDLE = 0x112;

	u32 monotonicMs()
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return (u32)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
	}

	// creates an anonymous file of the given size for wl_shm
	int createSharedMemoryFile(size_t size)
	{
		int fd = -1;
#ifdef MFD_CLOEXEC
		fd = memfd_create("irrlicht-shm", MFD_CLOEXEC);
#endif
		if (fd < 0)
		{
			const char* dir = getenv("XDG_RUNTIME_DIR");
			if (!dir)
				return -1;
			core::stringc name(dir);
			name += "/irrlicht-shm-XXXXXX";
			core::array<char> path;
			path.set_used(name.size() + 1);
			memcpy(path.pointer(), name.c_str(), name.size() + 1);
			fd = mkstemp(path.pointer());
			if (fd < 0)
				return -1;
			unlink(path.const_pointer());
			fcntl(fd, F_SETFD, FD_CLOEXEC);
		}
		if (ftruncate(fd, (off_t)size) < 0)
		{
			close(fd);
			return -1;
		}
		return fd;
	}

	void appendUtf8(core::stringc& out, u32 c)
	{
		if (c < 0x80)
			out.append((char)c);
		else if (c < 0x800)
		{
			out.append((char)(0xC0 | (c >> 6)));
			out.append((char)(0x80 | (c & 0x3F)));
		}
		else if (c < 0x10000)
		{
			out.append((char)(0xE0 | (c >> 12)));
			out.append((char)(0x80 | ((c >> 6) & 0x3F)));
			out.append((char)(0x80 | (c & 0x3F)));
		}
		else
		{
			out.append((char)(0xF0 | (c >> 18)));
			out.append((char)(0x80 | ((c >> 12) & 0x3F)));
			out.append((char)(0x80 | ((c >> 6) & 0x3F)));
			out.append((char)(0x80 | (c & 0x3F)));
		}
	}
}

//! Information about a wl_data_offer
struct SOfferInfo
{
	SOfferInfo() : MimeType(0) {}
	const char* MimeType; // best text mime type offered or 0
};

//! A shared memory buffer for software rendering
struct SShmBuffer
{
	SShmBuffer() : Buffer(0), Data(0), Size(0), Width(0), Height(0), Busy(false) {}
	struct wl_buffer* Buffer;
	void* Data;
	size_t Size;
	u32 Width, Height;
	bool Busy;
};

//! Everything related to Wayland, xkbcommon and EGL.
struct CIrrDeviceLinux::SWayland
{
	explicit SWayland(CIrrDeviceLinux* dev) : Dev(dev),
		Display(0), Registry(0), Compositor(0), Shm(0), DecorContext(0), Frame(0),
		Seat(0), Pointer(0), Keyboard(0), Output(0),
		DataDeviceManager(0), DataDevice(0), DataSource(0), SelectionOffer(0),
		Surface(0), EglWindow(0),
		CursorSurface(0), CursorTheme(0),
		XkbContext(0), XkbKeymap(0), XkbState(0),
		Configured(false),
		InputSerial(0), PointerEnterSerial(0), PointerInside(false),
		OutputWidth(0), OutputHeight(0),
		RepeatRate(25), RepeatDelay(600), RepeatKey(0), Repeating(false), NextRepeat(0),
		MouseX(0), MouseY(0), ButtonStates(0), AxisValue(0.f), AxisDiscrete(0), HasAxis(false),
		DiscardInput(false)
	{
	}

	CIrrDeviceLinux* Dev;

	struct wl_display* Display;
	struct wl_registry* Registry;
	struct wl_compositor* Compositor;
	struct wl_shm* Shm;
	struct libdecor* DecorContext; // draws the window decorations (title bar, buttons) if the compositor does not
	struct libdecor_frame* Frame;
	struct wl_seat* Seat;
	struct wl_pointer* Pointer;
	struct wl_keyboard* Keyboard;
	struct wl_output* Output;
	struct wl_data_device_manager* DataDeviceManager;
	struct wl_data_device* DataDevice;
	struct wl_data_source* DataSource;
	struct wl_data_offer* SelectionOffer;

	struct wl_surface* Surface;
	struct wl_egl_window* EglWindow;

	struct wl_surface* CursorSurface;
	struct wl_cursor_theme* CursorTheme;

	struct xkb_context* XkbContext;
	struct xkb_keymap* XkbKeymap;
	struct xkb_state* XkbState;

	bool Configured;

	u32 InputSerial;
	u32 PointerEnterSerial;
	bool PointerInside;

	s32 OutputWidth, OutputHeight;

	s32 RepeatRate, RepeatDelay;
	u32 RepeatKey;
	bool Repeating;
	u32 NextRepeat;

	s32 MouseX, MouseY;
	u32 ButtonStates;
	f32 AxisValue;
	s32 AxisDiscrete;
	bool HasAxis;

	bool DiscardInput;

	SShmBuffer Buffers[2];

	// ------------------------------------------------------------------
	// helpers
	// ------------------------------------------------------------------

	bool isModActive(const char* name) const
	{
		return XkbState && xkb_state_mod_name_is_active(XkbState, name, XKB_STATE_MODS_EFFECTIVE) > 0;
	}

	bool post(SEvent& e)
	{
		if (DiscardInput)
			return false;
		return Dev->postEventFromUser(e);
	}

	void postMouse(EMOUSE_INPUT_EVENT type, f32 wheel = 0.f)
	{
		SEvent e;
		e.EventType = EET_MOUSE_INPUT_EVENT;
		e.MouseInput.Event = type;
		e.MouseInput.X = MouseX;
		e.MouseInput.Y = MouseY;
		e.MouseInput.Wheel = wheel;
		e.MouseInput.Control = isModActive(XKB_MOD_NAME_CTRL);
		e.MouseInput.Shift = isModActive(XKB_MOD_NAME_SHIFT);
		e.MouseInput.ButtonStates = ButtonStates;
		post(e);
	}

	EKEY_CODE mapKey(xkb_keycode_t code)
	{
		EKEY_CODE keyCode = (EKEY_CODE)0;

		xkb_keysym_t sym = 0;
		const xkb_keysym_t* syms = 0;
		// same as the X11 device: symbol on the first layout without modifiers
		if (XkbKeymap && xkb_keymap_key_get_syms_by_level(XkbKeymap, code, 0, 0, &syms) > 0)
			sym = syms[0];

		SKeyMap mp(sym, 0);
		const s32 idx = Dev->KeyMap.binary_search(mp);
		if (idx != -1)
			keyCode = (EKEY_CODE)Dev->KeyMap[idx].Win32Key;

		if (keyCode == 0)
		{
			// Any value is better than none, that allows at least using the keys.
			if (!sym)
			{
				keyCode = (EKEY_CODE)code;
				os::Printer::log("No such keysym, using event keycode", core::stringc(code).c_str(), ELL_INFORMATION);
			}
			else
			{
				keyCode = (EKEY_CODE)sym;
				os::Printer::log("EKEY_CODE not found, using orig. keysym", core::stringc(sym).c_str(), ELL_INFORMATION);
			}
		}
		return keyCode;
	}

	void postKey(u32 evdevKey, bool pressed)
	{
		const xkb_keycode_t code = evdevKey + 8;
		SEvent e;
		e.EventType = EET_KEY_INPUT_EVENT;
		e.KeyInput.PressedDown = pressed;
		e.KeyInput.Char = 0;
		if (pressed && XkbState)
			e.KeyInput.Char = (wchar_t)xkb_state_key_get_utf32(XkbState, code);
		e.KeyInput.Control = isModActive(XKB_MOD_NAME_CTRL);
		e.KeyInput.Shift = isModActive(XKB_MOD_NAME_SHIFT);
		e.KeyInput.Key = mapKey(code);
		post(e);
	}

	// ------------------------------------------------------------------
	// shared memory buffers
	// ------------------------------------------------------------------

	// returns a mapped buffer, format is XRGB8888 / ARGB8888
	static bool createShmBuffer(struct wl_shm* shm, u32 width, u32 height, u32 format, SShmBuffer& out)
	{
		const size_t stride = (size_t)width * 4;
		const size_t size = stride * height;
		if (!shm || !size)
			return false;
		int fd = createSharedMemoryFile(size);
		if (fd < 0)
			return false;
		void* data = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (data == MAP_FAILED)
		{
			close(fd);
			return false;
		}
		struct wl_shm_pool* pool = wl_shm_create_pool(shm, fd, (s32)size);
		out.Buffer = wl_shm_pool_create_buffer(pool, 0, (s32)width, (s32)height, (s32)stride, format);
		wl_shm_pool_destroy(pool);
		close(fd);
		out.Data = data;
		out.Size = size;
		out.Width = width;
		out.Height = height;
		out.Busy = false;
		return out.Buffer != 0;
	}

	static void destroyShmBuffer(SShmBuffer& b)
	{
		if (b.Buffer)
			wl_buffer_destroy(b.Buffer);
		if (b.Data)
			munmap(b.Data, b.Size);
		b = SShmBuffer();
	}

	static void bufferRelease(void* data, struct wl_buffer*)
	{
		static_cast<SShmBuffer*>(data)->Busy = false;
	}

	void freeBuffers()
	{
		for (u32 i = 0; i < 2; ++i)
			destroyShmBuffer(Buffers[i]);
	}

	SShmBuffer* acquireBuffer(u32 width, u32 height)
	{
		for (u32 attempt = 0; attempt < 6; ++attempt)
		{
			for (u32 i = 0; i < 2; ++i)
			{
				SShmBuffer& b = Buffers[i];
				if (b.Busy)
					continue;
				if (b.Buffer && (b.Width != width || b.Height != height))
					destroyShmBuffer(b);
				if (!b.Buffer)
				{
					if (!createShmBuffer(Shm, width, height, WL_SHM_FORMAT_XRGB8888, b))
						return 0;
					static const struct wl_buffer_listener listener = { &SWayland::bufferRelease };
					wl_buffer_add_listener(b.Buffer, &listener, &b);
				}
				return &b;
			}
			// both buffers are still used by the compositor, wait for a release
			dispatch(20);
		}
		return 0;
	}

	// ------------------------------------------------------------------
	// event dispatching
	// ------------------------------------------------------------------

	// dispatches all pending events, waits at most timeoutMs for new ones. Returns false if the connection broke.
	bool dispatch(int timeoutMs)
	{
		if (!Display)
			return false;

		while (wl_display_prepare_read(Display) != 0)
			wl_display_dispatch_pending(Display);

		if (wl_display_flush(Display) < 0 && errno != EAGAIN)
		{
			wl_display_cancel_read(Display);
			return false;
		}

		struct pollfd pfd;
		pfd.fd = wl_display_get_fd(Display);
		pfd.events = POLLIN;
		pfd.revents = 0;
		if (poll(&pfd, 1, timeoutMs) > 0 && (pfd.revents & POLLIN))
		{
			if (wl_display_read_events(Display) < 0)
				return false;
		}
		else
			wl_display_cancel_read(Display);

		if (wl_display_dispatch_pending(Display) < 0)
			return false;
		if (DecorContext && libdecor_dispatch(DecorContext, 0) < 0)
			return false;
		return wl_display_get_error(Display) == 0;
	}

	// ------------------------------------------------------------------
	// registry
	// ------------------------------------------------------------------

	static void registryGlobal(void* data, struct wl_registry* registry, uint32_t name, const char* iface, uint32_t version)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		if (!strcmp(iface, wl_compositor_interface.name))
			wl->Compositor = (struct wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, version < 4 ? version : 4);
		else if (!strcmp(iface, wl_shm_interface.name))
			wl->Shm = (struct wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, 1);
		else if (!strcmp(iface, wl_seat_interface.name))
		{
			wl->Seat = (struct wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, version < 5 ? version : 5);
			static const struct wl_seat_listener listener = { &SWayland::seatCapabilities, &SWayland::seatName };
			wl_seat_add_listener(wl->Seat, &listener, wl);
		}
		else if (!strcmp(iface, wl_output_interface.name))
		{
			if (!wl->Output)
			{
				wl->Output = (struct wl_output*)wl_registry_bind(registry, name, &wl_output_interface, version < 2 ? version : 2);
				static const struct wl_output_listener listener = { &SWayland::outputGeometry, &SWayland::outputMode, &SWayland::outputDone, &SWayland::outputScale };
				wl_output_add_listener(wl->Output, &listener, wl);
			}
		}
		else if (!strcmp(iface, wl_data_device_manager_interface.name))
			wl->DataDeviceManager = (struct wl_data_device_manager*)wl_registry_bind(registry, name, &wl_data_device_manager_interface, version < 3 ? version : 3);
	}

	static void registryGlobalRemove(void*, struct wl_registry*, uint32_t) {}

	// ------------------------------------------------------------------
	// output
	// ------------------------------------------------------------------

	static void outputGeometry(void*, struct wl_output*, int32_t, int32_t, int32_t, int32_t, int32_t, const char*, const char*, int32_t) {}

	static void outputMode(void* data, struct wl_output*, uint32_t flags, int32_t width, int32_t height, int32_t)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		if (flags & WL_OUTPUT_MODE_CURRENT)
		{
			wl->OutputWidth = width;
			wl->OutputHeight = height;
		}
	}

	static void outputDone(void*, struct wl_output*) {}
	static void outputScale(void*, struct wl_output*, int32_t) {}

	// ------------------------------------------------------------------
	// libdecor
	// ------------------------------------------------------------------

	static void decorError(struct libdecor*, enum libdecor_error error, const char* message)
	{
		os::Printer::log("libdecor error", message, ELL_ERROR);
	}

	static void frameConfigure(struct libdecor_frame* frame, struct libdecor_configuration* configuration, void* data)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		int w = 0, h = 0;
		if (!libdecor_configuration_get_content_size(configuration, frame, &w, &h) || w <= 0 || h <= 0)
		{
			// the compositor leaves the size to us
			w = (int)wl->Dev->Width;
			h = (int)wl->Dev->Height;
		}

		struct libdecor_state* state = libdecor_state_new(w, h);
		libdecor_frame_commit(frame, state, configuration);
		libdecor_state_free(state);
		wl->Configured = true;

		if ((u32)w != wl->Dev->Width || (u32)h != wl->Dev->Height)
			wl->Dev->resizeWindow((u32)w, (u32)h);
	}

	static void frameClose(struct libdecor_frame*, void* data)
	{
		os::Printer::log("Quit message received.", ELL_INFORMATION);
		static_cast<SWayland*>(data)->Dev->Close = true;
	}

	static void frameCommit(struct libdecor_frame*, void* data)
	{
		wl_surface_commit(static_cast<SWayland*>(data)->Surface);
	}

	static void frameDismissPopup(struct libdecor_frame*, const char*, void*) {}

	// ------------------------------------------------------------------
	// seat
	// ------------------------------------------------------------------

	static void seatName(void*, struct wl_seat*, const char*) {}

	static void seatCapabilities(void* data, struct wl_seat* seat, uint32_t caps)
	{
		SWayland* wl = static_cast<SWayland*>(data);

		if ((caps & WL_SEAT_CAPABILITY_POINTER) && !wl->Pointer)
		{
			wl->Pointer = wl_seat_get_pointer(seat);
			static const struct wl_pointer_listener listener = {
				&SWayland::pointerEnter, &SWayland::pointerLeave, &SWayland::pointerMotion, &SWayland::pointerButton,
				&SWayland::pointerAxis, &SWayland::pointerFrame, &SWayland::pointerAxisSource, &SWayland::pointerAxisStop,
				&SWayland::pointerAxisDiscrete };
			wl_pointer_add_listener(wl->Pointer, &listener, wl);
		}
		else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && wl->Pointer)
		{
			wl_pointer_release(wl->Pointer);
			wl->Pointer = 0;
			wl->PointerInside = false;
		}

		if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !wl->Keyboard)
		{
			wl->Keyboard = wl_seat_get_keyboard(seat);
			static const struct wl_keyboard_listener listener = {
				&SWayland::keyboardKeymap, &SWayland::keyboardEnter, &SWayland::keyboardLeave, &SWayland::keyboardKey,
				&SWayland::keyboardModifiers, &SWayland::keyboardRepeatInfo };
			wl_keyboard_add_listener(wl->Keyboard, &listener, wl);
		}
		else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && wl->Keyboard)
		{
			wl_keyboard_release(wl->Keyboard);
			wl->Keyboard = 0;
			wl->Repeating = false;
		}
	}

	// ------------------------------------------------------------------
	// pointer
	// ------------------------------------------------------------------

	static void pointerEnter(void* data, struct wl_pointer*, uint32_t serial, struct wl_surface*, wl_fixed_t x, wl_fixed_t y)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		wl->PointerEnterSerial = serial;
		wl->PointerInside = true;
		wl->MouseX = wl_fixed_to_int(x);
		wl->MouseY = wl_fixed_to_int(y);
		static_cast<CCursorControl*>(wl->Dev->CursorControl)->setPositionFromCompositor(wl->MouseX, wl->MouseY);
		static_cast<CCursorControl*>(wl->Dev->CursorControl)->applyCursor(serial);
	}

	static void pointerLeave(void* data, struct wl_pointer*, uint32_t, struct wl_surface*)
	{
		static_cast<SWayland*>(data)->PointerInside = false;
	}

	static void pointerMotion(void* data, struct wl_pointer*, uint32_t, wl_fixed_t x, wl_fixed_t y)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		wl->MouseX = wl_fixed_to_int(x);
		wl->MouseY = wl_fixed_to_int(y);
		static_cast<CCursorControl*>(wl->Dev->CursorControl)->setPositionFromCompositor(wl->MouseX, wl->MouseY);
		wl->postMouse(EMIE_MOUSE_MOVED);
	}

	static void pointerButton(void* data, struct wl_pointer*, uint32_t serial, uint32_t, uint32_t button, uint32_t state)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		wl->InputSerial = serial;
		const bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);

		EMOUSE_INPUT_EVENT ev = EMIE_COUNT;
		u32 mask = 0;
		switch (button)
		{
		case WL_BTN_LEFT:
			ev = pressed ? EMIE_LMOUSE_PRESSED_DOWN : EMIE_LMOUSE_LEFT_UP;
			mask = EMBSM_LEFT;
			break;
		case WL_BTN_RIGHT:
			ev = pressed ? EMIE_RMOUSE_PRESSED_DOWN : EMIE_RMOUSE_LEFT_UP;
			mask = EMBSM_RIGHT;
			break;
		case WL_BTN_MIDDLE:
			ev = pressed ? EMIE_MMOUSE_PRESSED_DOWN : EMIE_MMOUSE_LEFT_UP;
			mask = EMBSM_MIDDLE;
			break;
		default:
			return;
		}

		if (pressed)
			wl->ButtonStates |= mask;
		else
			wl->ButtonStates &= ~mask;

		wl->postMouse(ev);

		if (pressed && !wl->DiscardInput)
		{
			const u32 clicks = wl->Dev->checkSuccessiveClicks(wl->MouseX, wl->MouseY, ev);
			if (clicks == 2)
				wl->postMouse((EMOUSE_INPUT_EVENT)(EMIE_LMOUSE_DOUBLE_CLICK + ev - EMIE_LMOUSE_PRESSED_DOWN));
			else if (clicks == 3)
				wl->postMouse((EMOUSE_INPUT_EVENT)(EMIE_LMOUSE_TRIPLE_CLICK + ev - EMIE_LMOUSE_PRESSED_DOWN));
		}
	}

	static void pointerAxis(void* data, struct wl_pointer*, uint32_t, uint32_t axis, wl_fixed_t value)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
		{
			wl->AxisValue += (f32)wl_fixed_to_double(value);
			wl->HasAxis = true;
		}
	}

	static void pointerAxisDiscrete(void* data, struct wl_pointer*, uint32_t axis, int32_t discrete)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
			wl->AxisDiscrete += discrete;
	}

	static void pointerAxisSource(void*, struct wl_pointer*, uint32_t) {}
	static void pointerAxisStop(void*, struct wl_pointer*, uint32_t, uint32_t) {}

	// all axis events of one logical scroll arrive before the frame event
	static void pointerFrame(void* data, struct wl_pointer*)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		if (wl->AxisDiscrete != 0)
			wl->postMouse(EMIE_MOUSE_WHEEL, -(f32)wl->AxisDiscrete);
		else if (wl->HasAxis && wl->AxisValue != 0.f)
			wl->postMouse(EMIE_MOUSE_WHEEL, -wl->AxisValue / 10.f); // about 10 units per wheel notch
		wl->AxisDiscrete = 0;
		wl->AxisValue = 0.f;
		wl->HasAxis = false;
	}

	// ------------------------------------------------------------------
	// keyboard
	// ------------------------------------------------------------------

	static void keyboardKeymap(void* data, struct wl_keyboard*, uint32_t format, int32_t fd, uint32_t size)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
		{
			close(fd);
			return;
		}
		void* map = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
		close(fd);
		if (map == MAP_FAILED)
			return;

		struct xkb_keymap* keymap = xkb_keymap_new_from_string(wl->XkbContext, (const char*)map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
		munmap(map, size);
		if (!keymap)
		{
			os::Printer::log("Could not compile the keymap sent by the compositor.", ELL_WARNING);
			return;
		}
		struct xkb_state* state = xkb_state_new(keymap);
		if (wl->XkbState)
			xkb_state_unref(wl->XkbState);
		if (wl->XkbKeymap)
			xkb_keymap_unref(wl->XkbKeymap);
		wl->XkbKeymap = keymap;
		wl->XkbState = state;
	}

	static void keyboardEnter(void* data, struct wl_keyboard*, uint32_t serial, struct wl_surface*, struct wl_array*)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		wl->InputSerial = serial;
		wl->Dev->WindowHasFocus = true;
	}

	static void keyboardLeave(void* data, struct wl_keyboard*, uint32_t, struct wl_surface*)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		wl->Dev->WindowHasFocus = false;
		wl->Repeating = false;
	}

	static void keyboardKey(void* data, struct wl_keyboard*, uint32_t serial, uint32_t, uint32_t key, uint32_t state)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		wl->InputSerial = serial;
		const bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);
		wl->postKey(key, pressed);

		if (pressed)
		{
			if (wl->XkbKeymap && wl->RepeatRate > 0 && xkb_keymap_key_repeats(wl->XkbKeymap, key + 8))
			{
				wl->Repeating = true;
				wl->RepeatKey = key;
				wl->NextRepeat = monotonicMs() + (u32)wl->RepeatDelay;
			}
		}
		else if (wl->Repeating && wl->RepeatKey == key)
			wl->Repeating = false;
	}

	static void keyboardModifiers(void* data, struct wl_keyboard*, uint32_t, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		if (wl->XkbState)
			xkb_state_update_mask(wl->XkbState, depressed, latched, locked, 0, 0, group);
	}

	static void keyboardRepeatInfo(void* data, struct wl_keyboard*, int32_t rate, int32_t delay)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		wl->RepeatRate = rate;
		wl->RepeatDelay = delay;
	}

	// Wayland has no server side key repeat
	void processKeyRepeat()
	{
		if (!Repeating || RepeatRate <= 0)
			return;
		const u32 interval = (u32)(1000 / RepeatRate) ? (u32)(1000 / RepeatRate) : 1;
		u32 count = 0;
		u32 now = monotonicMs();
		while ((s32)(now - NextRepeat) >= 0 && Repeating && count < 10)
		{
			// like on Windows and X11 a repeated key is reported as another key press
			postKey(RepeatKey, true);
			NextRepeat += interval;
			++count;
		}
		if (count == 10)
			NextRepeat = now + interval;
	}

	// ------------------------------------------------------------------
	// clipboard
	// ------------------------------------------------------------------

	static void offerOffer(void* data, struct wl_data_offer*, const char* mime)
	{
		SOfferInfo* info = static_cast<SOfferInfo*>(data);
		static const char* const preferred[] = { "text/plain;charset=utf-8", "UTF8_STRING", "text/plain", "STRING", "TEXT", 0 };
		// the earlier in the list, the better
		int currentRank = 5;
		int newRank = 5;
		for (int i = 0; preferred[i]; ++i)
		{
			if (info->MimeType && !strcmp(info->MimeType, preferred[i]))
				currentRank = i;
			if (!strcmp(mime, preferred[i]))
				newRank = i;
		}
		if (newRank < currentRank)
		{
			// store the static string, the mime pointer is only valid during the callback
			info->MimeType = preferred[newRank];
		}
	}

	static void offerSourceActions(void*, struct wl_data_offer*, uint32_t) {}
	static void offerAction(void*, struct wl_data_offer*, uint32_t) {}

	static void destroyOffer(struct wl_data_offer* offer)
	{
		if (!offer)
			return;
		delete static_cast<SOfferInfo*>(wl_data_offer_get_user_data(offer));
		wl_data_offer_destroy(offer);
	}

	static void deviceDataOffer(void*, struct wl_data_device*, struct wl_data_offer* offer)
	{
		static const struct wl_data_offer_listener listener = { &SWayland::offerOffer, &SWayland::offerSourceActions, &SWayland::offerAction };
		wl_data_offer_add_listener(offer, &listener, new SOfferInfo());
	}

	static void deviceEnter(void*, struct wl_data_device*, uint32_t, struct wl_surface*, wl_fixed_t, wl_fixed_t, struct wl_data_offer* offer)
	{
		destroyOffer(offer); // drag and drop is not supported
	}

	static void deviceLeave(void*, struct wl_data_device*) {}
	static void deviceMotion(void*, struct wl_data_device*, uint32_t, wl_fixed_t, wl_fixed_t) {}
	static void deviceDrop(void*, struct wl_data_device*) {}

	static void deviceSelection(void* data, struct wl_data_device*, struct wl_data_offer* offer)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		destroyOffer(wl->SelectionOffer);
		wl->SelectionOffer = offer;
	}

	static void sourceTarget(void*, struct wl_data_source*, const char*) {}

	static void sourceSend(void* data, struct wl_data_source*, const char*, int32_t fd)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		const core::stringc& text = wl->Dev->Clipboard;
		const char* p = text.c_str();
		size_t left = text.size();
		while (left > 0)
		{
			ssize_t written = write(fd, p, left);
			if (written < 0)
			{
				if (errno == EINTR)
					continue;
				break;
			}
			p += written;
			left -= (size_t)written;
		}
		close(fd);
	}

	static void sourceCancelled(void* data, struct wl_data_source* source)
	{
		SWayland* wl = static_cast<SWayland*>(data);
		if (wl->DataSource == source)
			wl->DataSource = 0;
		wl_data_source_destroy(source);
	}

	static void sourceDndDropPerformed(void*, struct wl_data_source*) {}
	static void sourceDndFinished(void*, struct wl_data_source*) {}
	static void sourceAction(void*, struct wl_data_source*, uint32_t) {}

	void setupDataDevice()
	{
		if (!DataDeviceManager || !Seat || DataDevice)
			return;
		DataDevice = wl_data_device_manager_get_data_device(DataDeviceManager, Seat);
		static const struct wl_data_device_listener listener = {
			&SWayland::deviceDataOffer, &SWayland::deviceEnter, &SWayland::deviceLeave,
			&SWayland::deviceMotion, &SWayland::deviceDrop, &SWayland::deviceSelection };
		wl_data_device_add_listener(DataDevice, &listener, this);
	}

	// ------------------------------------------------------------------
	// teardown
	// ------------------------------------------------------------------

	void destroyWindowObjects()
	{
		freeBuffers();
		if (EglWindow)
			wl_egl_window_destroy(EglWindow);
		EglWindow = 0;
		if (Frame)
			libdecor_frame_unref(Frame);
		Frame = 0;
		if (Surface)
			wl_surface_destroy(Surface);
		Surface = 0;
	}

	void destroyAll()
	{
		destroyWindowObjects();

		if (DataSource)
			wl_data_source_destroy(DataSource);
		DataSource = 0;
		destroyOffer(SelectionOffer);
		SelectionOffer = 0;
		if (DataDevice)
			wl_data_device_destroy(DataDevice);
		DataDevice = 0;
		if (DataDeviceManager)
			wl_data_device_manager_destroy(DataDeviceManager);
		DataDeviceManager = 0;

		if (CursorSurface)
			wl_surface_destroy(CursorSurface);
		CursorSurface = 0;
		if (CursorTheme)
			wl_cursor_theme_destroy(CursorTheme);
		CursorTheme = 0;

		if (Pointer)
			wl_pointer_release(Pointer);
		Pointer = 0;
		if (Keyboard)
			wl_keyboard_release(Keyboard);
		Keyboard = 0;
		if (Seat)
			wl_seat_destroy(Seat);
		Seat = 0;
		if (Output)
			wl_output_destroy(Output);
		Output = 0;

		if (DecorContext)
			libdecor_unref(DecorContext);
		DecorContext = 0;
		if (Shm)
			wl_shm_destroy(Shm);
		Shm = 0;
		if (Compositor)
			wl_compositor_destroy(Compositor);
		Compositor = 0;
		if (Registry)
			wl_registry_destroy(Registry);
		Registry = 0;

		if (XkbState)
			xkb_state_unref(XkbState);
		XkbState = 0;
		if (XkbKeymap)
			xkb_keymap_unref(XkbKeymap);
		XkbKeymap = 0;
		if (XkbContext)
			xkb_context_unref(XkbContext);
		XkbContext = 0;

		if (Display)
		{
			wl_display_flush(Display);
			wl_display_disconnect(Display);
		}
		Display = 0;
	}
};


//! constructor
CIrrDeviceLinux::CIrrDeviceLinux(const SIrrlichtCreationParameters& param)
	: CIrrDeviceStub(param), Wl(0),
	Width(param.WindowSize.Width), Height(param.WindowSize.Height),
	WindowHasFocus(true), WindowMinimized(false),
	ExternalWindow(false)
{
	#ifdef _DEBUG
	setDebugName("CIrrDeviceLinux");
	#endif

	// print version, distribution etc.
	core::stringc linuxversion;
	struct utsname LinuxInfo;
	uname(&LinuxInfo);

	linuxversion += LinuxInfo.sysname;
	linuxversion += " ";
	linuxversion += LinuxInfo.release;
	linuxversion += " ";
	linuxversion += LinuxInfo.version;
	linuxversion += " ";
	linuxversion += LinuxInfo.machine;

	Operator = new COSOperator(linuxversion, this);
	os::Printer::log(linuxversion.c_str(), ELL_INFORMATION);

	// create keymap
	createKeyMap();

	// create window
	if (CreationParams.DriverType != video::EDT_NULL)
	{
		// create the window, only if we do not use the null device
		if (!createWindow())
			return;
		setResizable(param.WindowResizable);
	}

	// create cursor control
	CursorControl = new CCursorControl(this, CreationParams.DriverType == video::EDT_NULL);

	// create driver
	createDriver();

	if (!VideoDriver)
		return;

	createGUIAndScene();
}


//! destructor
CIrrDeviceLinux::~CIrrDeviceLinux()
{
	if (CursorControl)
		static_cast<CCursorControl*>(CursorControl)->clearCursors();

	// Must free OpenGL textures etc before destroying context, so can't wait for stub destructor
	if ( GUIEnvironment )
	{
		GUIEnvironment->drop();
		GUIEnvironment = NULL;
	}
	if ( SceneManager )
	{
		SceneManager->drop();
		SceneManager = NULL;
	}
	if ( VideoDriver )
	{
		VideoDriver->drop();
		VideoDriver = NULL;
	}

	if (Wl)
	{
		if (ContextManager)
		{
			ContextManager->destroyContext();
			ContextManager->destroySurface();
			ContextManager->terminate();
		}
		Wl->destroyAll();
		delete Wl;
		Wl = 0;
	}

#if defined(_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
	for (u32 joystick = 0; joystick < ActiveJoysticks.size(); ++joystick)
	{
		if (ActiveJoysticks[joystick].fd >= 0)
		{
			close(ActiveJoysticks[joystick].fd);
		}
	}
#endif
}


bool CIrrDeviceLinux::createWindow()
{
	Wl = new SWayland(this);

	Wl->Display = wl_display_connect(NULL);
	if (!Wl->Display)
	{
		os::Printer::log("Error: Need a running Wayland compositor to start Irrlicht Engine.", ELL_ERROR);
		os::Printer::log("Could not connect to the Wayland display, check WAYLAND_DISPLAY and XDG_RUNTIME_DIR", ELL_ERROR);
		delete Wl;
		Wl = 0;
		return false;
	}

	Wl->XkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	Wl->Registry = wl_display_get_registry(Wl->Display);
	static const struct wl_registry_listener registryListener = { &SWayland::registryGlobal, &SWayland::registryGlobalRemove };
	wl_registry_add_listener(Wl->Registry, &registryListener, Wl);
	wl_display_roundtrip(Wl->Display); // globals
	wl_display_roundtrip(Wl->Display); // seat capabilities, output modes, ...

	if (!Wl->Compositor || !Wl->Shm)
	{
		os::Printer::log("The Wayland compositor does not support wl_compositor and wl_shm.", ELL_ERROR);
		Wl->destroyAll();
		delete Wl;
		Wl = 0;
		return false;
	}

	Wl->setupDataDevice();

	static struct libdecor_interface decorInterface = { &SWayland::decorError };
	Wl->DecorContext = libdecor_new(Wl->Display, &decorInterface);
	if (!Wl->DecorContext)
	{
		os::Printer::log("Could not create the libdecor context (xdg_wm_base required).", ELL_ERROR);
		Wl->destroyAll();
		delete Wl;
		Wl = 0;
		return false;
	}

	Wl->Surface = wl_compositor_create_surface(Wl->Compositor);
	static struct libdecor_frame_interface frameInterface = {
		&SWayland::frameConfigure, &SWayland::frameClose, &SWayland::frameCommit, &SWayland::frameDismissPopup };
	Wl->Frame = libdecor_decorate(Wl->DecorContext, Wl->Surface, &frameInterface, Wl);
	libdecor_frame_set_title(Wl->Frame, "Irrlicht");
	libdecor_frame_set_app_id(Wl->Frame, "irrlicht");
	if (CreationParams.Fullscreen)
		libdecor_frame_set_fullscreen(Wl->Frame, NULL);
	libdecor_frame_map(Wl->Frame);

	// wait for the first configure, it triggers the creation of the decorations
	while (!Wl->Configured)
	{
		if (!Wl->dispatch(100))
		{
			os::Printer::log("Lost connection to the Wayland compositor while creating the window.", ELL_ERROR);
			Wl->destroyAll();
			delete Wl;
			Wl = 0;
			return false;
		}
	}
	// the compositor can have chosen another size (e.g. fullscreen), the driver must use it
	CreationParams.WindowSize.Width = Width;
	CreationParams.WindowSize.Height = Height;

	if (CreationParams.DriverType != video::EDT_SOFTWARE && CreationParams.DriverType != video::EDT_BURNINGSVIDEO)
	{
		Wl->EglWindow = wl_egl_window_create(Wl->Surface, (int)Width, (int)Height);
		if (!Wl->EglWindow)
		{
			os::Printer::log("Could not create the wl_egl_window.", ELL_ERROR);
			Wl->destroyAll();
			delete Wl;
			Wl = 0;
			return false;
		}
	}

	Wl->CursorSurface = wl_compositor_create_surface(Wl->Compositor);
	Wl->CursorTheme = wl_cursor_theme_load(NULL, 24, Wl->Shm);
	if (!Wl->CursorTheme)
		os::Printer::log("Could not load a cursor theme, cursor icons are not available.", ELL_WARNING);

	if (Wl->OutputWidth > 0 && Wl->OutputHeight > 0 && VideoModeList)
		VideoModeList->setDesktop(24, core::dimension2d<u32>((u32)Wl->OutputWidth, (u32)Wl->OutputHeight));

	return true;
}


//! create the driver
void CIrrDeviceLinux::createDriver()
{
	switch(CreationParams.DriverType)
	{
	case video::EDT_SOFTWARE:
#ifdef _IRR_COMPILE_WITH_SOFTWARE_
		VideoDriver = video::createSoftwareDriver(CreationParams.WindowSize, CreationParams.Fullscreen, FileSystem, this);
#else
		os::Printer::log("No Software driver support compiled in.", ELL_ERROR);
#endif
		break;
	case video::EDT_BURNINGSVIDEO:
#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
		VideoDriver = video::createBurningVideoDriver(CreationParams, FileSystem, this);
#else
		os::Printer::log("Burning's video driver was not compiled in.", ELL_ERROR);
#endif
		break;
	case video::EDT_OPENGL:
#ifdef _IRR_COMPILE_WITH_OPENGL_
		{
			video::SExposedVideoData data;
			data.OpenGLLinux.X11Window = (unsigned long)Wl->EglWindow;
			data.OpenGLLinux.X11Display = Wl->Display;

			ContextManager = new video::CEGLManager();
			ContextManager->initialize(CreationParams, data);

			VideoDriver = video::createOpenGLDriver(CreationParams, FileSystem, ContextManager);
		}
#else
		os::Printer::log("No OpenGL support compiled in.", ELL_ERROR);
#endif
		break;
	case video::EDT_OGLES1:
#ifdef _IRR_COMPILE_WITH_OGLES1_
		{
			video::SExposedVideoData data;
			data.OpenGLLinux.X11Window = (unsigned long)Wl->EglWindow;
			data.OpenGLLinux.X11Display = Wl->Display;

			ContextManager = new video::CEGLManager();
			ContextManager->initialize(CreationParams, data);

			VideoDriver = video::createOGLES1Driver(CreationParams, FileSystem, ContextManager);
		}
#else
		os::Printer::log("No OpenGL-ES1 support compiled in.", ELL_ERROR);
#endif
		break;
	case video::EDT_OGLES2:
#ifdef _IRR_COMPILE_WITH_OGLES2_
		{
			video::SExposedVideoData data;
			data.OpenGLLinux.X11Window = (unsigned long)Wl->EglWindow;
			data.OpenGLLinux.X11Display = Wl->Display;

			ContextManager = new video::CEGLManager();
			ContextManager->initialize(CreationParams, data);

			VideoDriver = video::createOGLES2Driver(CreationParams, FileSystem, ContextManager);
		}
#else
		os::Printer::log("No OpenGL-ES2 support compiled in.", ELL_ERROR);
#endif
		break;
	case video::EDT_WEBGL1:
#ifdef _IRR_COMPILE_WITH_WEBGL1_
		{
			video::SExposedVideoData data;
			data.OpenGLLinux.X11Window = (unsigned long)Wl->EglWindow;
			data.OpenGLLinux.X11Display = Wl->Display;

			ContextManager = new video::CEGLManager();
			ContextManager->initialize(CreationParams, data);

			VideoDriver = video::createWebGL1Driver(CreationParams, FileSystem, ContextManager);
		}
#else
		os::Printer::log("No WebGL1 support compiled in.", ELL_ERROR);
#endif
		break;
	case video::DEPRECATED_EDT_DIRECT3D8_NO_LONGER_EXISTS:
	case video::EDT_DIRECT3D9:
		os::Printer::log("This driver is not available in Linux. Try OpenGL or Software renderer.",
			ELL_ERROR);
		break;
	case video::EDT_NULL:
		VideoDriver = video::createNullDriver(FileSystem, CreationParams.WindowSize);
		break;
	default:
		os::Printer::log("Unable to create video driver of unknown type.", ELL_ERROR);
		break;
	}
}


//! called when the compositor or the application changes the window size
void CIrrDeviceLinux::resizeWindow(u32 width, u32 height)
{
	Width = width;
	Height = height;

	if (Wl)
	{
		if (Wl->EglWindow)
			wl_egl_window_resize(Wl->EglWindow, (int)width, (int)height, 0, 0);
		// shared memory buffers get recreated with the new size on the next present
		Wl->freeBuffers();
	}

	if (VideoDriver)
		VideoDriver->OnResize(core::dimension2d<u32>(Width, Height));
}


//! runs the device. Returns false if device wants to be deleted
bool CIrrDeviceLinux::run()
{
	os::Timer::tick();

	if ( CursorControl )
		static_cast<CCursorControl*>(CursorControl)->update();

	if ((CreationParams.DriverType != video::EDT_NULL) && Wl && Wl->Display)
	{
		if (!Wl->dispatch(0))
		{
			if (wl_display_get_error(Wl->Display))
				os::Printer::log("Wayland connection error, closing the device.", ELL_ERROR);
			Close = true;
		}
		else
			Wl->processKeyRepeat();
	}

	if (!Close)
		pollJoysticks();

	return !Close;
}


//! Pause the current process for the minimum time allowed only to allow other processes to execute
void CIrrDeviceLinux::yield()
{
	struct timespec ts = {0,1};
	nanosleep(&ts, NULL);
}


//! Pause execution and let other processes to run for a specified amount of time.
void CIrrDeviceLinux::sleep(u32 timeMs, bool pauseTimer=false)
{
	const bool wasStopped = Timer ? Timer->isStopped() : true;

	struct timespec ts;
	ts.tv_sec = (time_t) (timeMs / 1000);
	ts.tv_nsec = (long) (timeMs % 1000) * 1000000;

	if (pauseTimer && !wasStopped)
		Timer->stop();

	nanosleep(&ts, NULL);

	if (pauseTimer && !wasStopped)
		Timer->start();
}


//! sets the caption of the window
void CIrrDeviceLinux::setWindowCaption(const wchar_t* text)
{
	if (CreationParams.DriverType == video::EDT_NULL || !Wl || !Wl->Frame)
		return;

	core::stringc utf8;
	for (const wchar_t* p = text; p && *p; ++p)
		appendUtf8(utf8, (u32)*p);
	libdecor_frame_set_title(Wl->Frame, utf8.c_str());
	wl_display_flush(Wl->Display);
}


//! presents a surface in the client area
bool CIrrDeviceLinux::present(video::IImage* image, void* windowId, core::rect<s32>* srcRect)
{
	// this is only necessary for software drivers.
	if (!Wl || !Wl->Surface || CreationParams.DriverType == video::EDT_NULL || Wl->EglWindow)
		return true;

	SShmBuffer* buffer = Wl->acquireBuffer(Width, Height);
	if (!buffer)
	{
		os::Printer::log("Could not get a shared memory buffer for presenting.", ELL_WARNING);
		return false;
	}

	const u32 destPitch = Width * 4;
	const u32 minWidth = core::min_(image->getDimension().Width, Width);
	const u32 rows = core::min_(image->getDimension().Height, Height);
	const u32 srcPitch = image->getPitch();
	const u8* srcdata = reinterpret_cast<const u8*>(image->getData());
	u8* destData = reinterpret_cast<u8*>(buffer->Data);

	for (u32 y = 0; y != rows; ++y)
	{
		video::CColorConverter::convert_viaFormat(srcdata, image->getColorFormat(), minWidth, destData, video::ECF_A8R8G8B8);
		srcdata += srcPitch;
		destData += destPitch;
	}

	buffer->Busy = true;
	wl_surface_attach(Wl->Surface, buffer->Buffer, 0, 0);
	wl_surface_damage_buffer(Wl->Surface, 0, 0, (s32)Width, (s32)Height);
	wl_surface_commit(Wl->Surface);
	wl_display_flush(Wl->Display);
	return true;
}


//! notifies the device that it should close itself
void CIrrDeviceLinux::closeDevice()
{
	Close = true;
}


//! returns if window is active. if not, nothing need to be drawn
bool CIrrDeviceLinux::isWindowActive() const
{
	return (WindowHasFocus && !WindowMinimized);
}


//! returns if window has focus.
bool CIrrDeviceLinux::isWindowFocused() const
{
	return WindowHasFocus;
}


//! returns if window is minimized.
bool CIrrDeviceLinux::isWindowMinimized() const
{
	return WindowMinimized;
}


//! returns color format of the window.
video::ECOLOR_FORMAT CIrrDeviceLinux::getColorFormat() const
{
	return video::ECF_A8R8G8B8;
}


//! Sets if the window should be resizable in windowed mode.
void CIrrDeviceLinux::setResizable(bool resize)
{
	if (CreationParams.DriverType == video::EDT_NULL || !Wl || !Wl->Frame || CreationParams.Fullscreen)
		return;

	// without the resize capability the decorations have neither a maximize button nor resize borders
	if (resize)
	{
		libdecor_frame_set_capabilities(Wl->Frame, LIBDECOR_ACTION_RESIZE);
		libdecor_frame_set_min_content_size(Wl->Frame, 0, 0);
		libdecor_frame_set_max_content_size(Wl->Frame, 0, 0);
	}
	else
	{
		libdecor_frame_unset_capabilities(Wl->Frame, LIBDECOR_ACTION_RESIZE);
		libdecor_frame_set_min_content_size(Wl->Frame, (s32)Width, (s32)Height);
		libdecor_frame_set_max_content_size(Wl->Frame, (s32)Width, (s32)Height);
	}
	wl_display_flush(Wl->Display);
}


//! Resize the render window.
void CIrrDeviceLinux::setWindowSize(const irr::core::dimension2d<u32>& size)
{
	if (CreationParams.DriverType == video::EDT_NULL || !Wl || !Wl->Frame)
		return;

	if (size.Width == 0 || size.Height == 0 || (size.Width == Width && size.Height == Height))
		return;

	// A Wayland client decides the size of its own surface, the compositor can still override it.
	resizeWindow(size.Width, size.Height);
	struct libdecor_state* state = libdecor_state_new((int)size.Width, (int)size.Height);
	libdecor_frame_commit(Wl->Frame, state, NULL);
	libdecor_state_free(state);
	wl_display_flush(Wl->Display);
}


//! Return pointer to a list with all video modes supported by the gfx adapter.
video::IVideoModeList* CIrrDeviceLinux::getVideoModeList()
{
	if (!VideoModeList->getVideoModeCount())
	{
		bool temporaryConnection = false;
		SWayland* wl = Wl;

		if (!wl)
		{
			wl = new SWayland(this);
			wl->Display = wl_display_connect(NULL);
			temporaryConnection = true;
			if (wl->Display)
			{
				wl->Registry = wl_display_get_registry(wl->Display);
				static const struct wl_registry_listener registryListener = { &SWayland::registryGlobal, &SWayland::registryGlobalRemove };
				wl_registry_add_listener(wl->Registry, &registryListener, wl);
				wl_display_roundtrip(wl->Display);
				wl_display_roundtrip(wl->Display);
			}
		}

		// Wayland only reports the current mode of an output
		if (wl->Display && wl->OutputWidth > 0 && wl->OutputHeight > 0)
		{
			const core::dimension2d<u32> desktop((u32)wl->OutputWidth, (u32)wl->OutputHeight);
			VideoModeList->setDesktop(24, desktop);
			VideoModeList->addMode(desktop, 24);
		}
		else
			os::Printer::log("Could not determine the output resolution for VideoModeList.", ELL_WARNING);

		if (temporaryConnection)
		{
			wl->destroyAll();
			delete wl;
		}
	}

	return VideoModeList;
}


//! Minimizes the window.
void CIrrDeviceLinux::minimizeWindow()
{
	if (CreationParams.DriverType == video::EDT_NULL || !Wl || !Wl->Frame)
		return;
	libdecor_frame_set_minimized(Wl->Frame);
	wl_display_flush(Wl->Display);
}


//! Maximizes the window.
void CIrrDeviceLinux::maximizeWindow()
{
	if (CreationParams.DriverType == video::EDT_NULL || !Wl || !Wl->Frame)
		return;
	libdecor_frame_set_maximized(Wl->Frame);
	wl_display_flush(Wl->Display);
}


//! Restore the window to normal size if possible.
void CIrrDeviceLinux::restoreWindow()
{
	if (CreationParams.DriverType == video::EDT_NULL || !Wl || !Wl->Frame)
		return;
	libdecor_frame_unset_maximized(Wl->Frame);
	wl_display_flush(Wl->Display);
}


core::position2di CIrrDeviceLinux::getWindowPosition()
{
	// Wayland clients do not know where their window is placed
	return core::position2di(0, 0);
}


void CIrrDeviceLinux::createKeyMap()
{
	KeyMap.reallocate(190);
#include "wayland/WaylandKeyMap.inc"
	KeyMap.sort();
}


bool CIrrDeviceLinux::activateJoysticks(core::array<SJoystickInfo> & joystickInfo)
{
#if defined (_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)

	joystickInfo.clear();

	u32 joystick;
	for (joystick = 0; joystick < 32; ++joystick)
	{
		// The joystick device could be here...
		core::stringc devName = "/dev/js";
		devName += joystick;

		SJoystickInfo returnInfo;
		JoystickInfo info;

		info.fd = open(devName.c_str(), O_RDONLY);
		if (-1 == info.fd)
		{
			// ...but Ubuntu and possibly other distros
			// create the devices in /dev/input
			devName = "/dev/input/js";
			devName += joystick;
			info.fd = open(devName.c_str(), O_RDONLY);
		}

		if (-1 == info.fd)
			continue;

		ioctl( info.fd, JSIOCGAXES, &(info.axes) );
		ioctl( info.fd, JSIOCGBUTTONS, &(info.buttons) );
		fcntl( info.fd, F_SETFL, O_NONBLOCK );

		(void)memset(&info.persistentData, 0, sizeof(info.persistentData));
		info.persistentData.EventType = irr::EET_JOYSTICK_INPUT_EVENT;
		info.persistentData.JoystickEvent.Joystick = ActiveJoysticks.size();

		// There's no obvious way to determine which (if any) axes represent a POV
		// hat, so we'll just set it to "not used" and forget about it.
		info.persistentData.JoystickEvent.POV = 65535;

		ActiveJoysticks.push_back(info);

		returnInfo.Joystick = joystick;
		returnInfo.PovHat = SJoystickInfo::POV_HAT_UNKNOWN;
		returnInfo.Axes = info.axes;
		returnInfo.Buttons = info.buttons;

		char name[80];
		ioctl( info.fd, JSIOCGNAME(80), name);
		returnInfo.Name = name;

		joystickInfo.push_back(returnInfo);
	}

	for (joystick = 0; joystick < joystickInfo.size(); ++joystick)
	{
		char logString[256];
		(void)sprintf(logString, "Found joystick %u, %u axes, %u buttons '%s'",
			joystick, joystickInfo[joystick].Axes,
			joystickInfo[joystick].Buttons, joystickInfo[joystick].Name.c_str());
		os::Printer::log(logString, ELL_INFORMATION);
	}

	return true;
#else
	return false;
#endif // _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
}


void CIrrDeviceLinux::pollJoysticks()
{
#if defined (_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
	if (0 == ActiveJoysticks.size())
		return;

	for (u32 j= 0; j< ActiveJoysticks.size(); ++j)
	{
		JoystickInfo & info =  ActiveJoysticks[j];

		struct js_event event;
		while (sizeof(event) == read(info.fd, &event, sizeof(event)))
		{
			switch(event.type & ~JS_EVENT_INIT)
			{
			case JS_EVENT_BUTTON:
				if (event.value)
						info.persistentData.JoystickEvent.ButtonStates |= (1 << event.number);
				else
						info.persistentData.JoystickEvent.ButtonStates &= ~(1 << event.number);
				break;

			case JS_EVENT_AXIS:
				if (event.number < SEvent::SJoystickEvent::NUMBER_OF_AXES)
					info.persistentData.JoystickEvent.Axis[event.number] = event.value;
				break;

			default:
				break;
			}
		}

		// Send an irrlicht joystick event once per ::run() even if no new data were received.
		(void)postEventFromUser(info.persistentData);
	}
#endif // _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
}


//! Gamma ramps can not be changed by Wayland clients
bool CIrrDeviceLinux::setGammaRamp( f32 red, f32 green, f32 blue, f32 brightness, f32 contrast )
{
	return false;
}


bool CIrrDeviceLinux::getGammaRamp( f32 &red, f32 &green, f32 &blue, f32 &brightness, f32 &contrast )
{
	return false;
}


//! gets text from the clipboard
//! \return Returns 0 if no string is in there.
const c8* CIrrDeviceLinux::getTextFromClipboard() const
{
	if (!Wl)
		return 0;

	// we own the selection
	if (Wl->DataSource)
		return Clipboard.c_str();

	Clipboard = "";
	if (!Wl->SelectionOffer)
		return Clipboard.c_str();

	SOfferInfo* info = static_cast<SOfferInfo*>(wl_data_offer_get_user_data(Wl->SelectionOffer));
	if (!info || !info->MimeType)
		return Clipboard.c_str();

	int fds[2];
	if (pipe2(fds, O_CLOEXEC) < 0)
		return Clipboard.c_str();

	wl_data_offer_receive(Wl->SelectionOffer, info->MimeType, fds[1]);
	close(fds[1]);
	wl_display_flush(Wl->Display);

	// the owner of the selection writes into the pipe from its own process
	char buffer[1024];
	for (u32 waited = 0; waited < 1000; )
	{
		struct pollfd pfd;
		pfd.fd = fds[0];
		pfd.events = POLLIN;
		pfd.revents = 0;
		const int ready = poll(&pfd, 1, 100);
		if (ready < 0 && errno != EINTR)
			break;
		if (ready <= 0)
		{
			waited += 100;
			continue;
		}
		const ssize_t n = read(fds[0], buffer, sizeof(buffer) - 1);
		if (n <= 0)
			break;
		buffer[n] = 0;
		Clipboard += buffer;
	}
	close(fds[0]);

	return Clipboard.c_str();
}


//! copies text to the clipboard
void CIrrDeviceLinux::copyToClipboard(const c8* text) const
{
	if (!Wl || !Wl->DataDeviceManager || !Wl->DataDevice)
		return;

	Clipboard = text;

	if (Wl->DataSource)
	{
		wl_data_source_destroy(Wl->DataSource);
		Wl->DataSource = 0;
	}

	Wl->DataSource = wl_data_device_manager_create_data_source(Wl->DataDeviceManager);
	static const struct wl_data_source_listener listener = {
		&SWayland::sourceTarget, &SWayland::sourceSend, &SWayland::sourceCancelled,
		&SWayland::sourceDndDropPerformed, &SWayland::sourceDndFinished, &SWayland::sourceAction };
	wl_data_source_add_listener(Wl->DataSource, &listener, Wl);
	wl_data_source_offer(Wl->DataSource, "text/plain;charset=utf-8");
	wl_data_source_offer(Wl->DataSource, "text/plain");
	wl_data_source_offer(Wl->DataSource, "UTF8_STRING");
	wl_data_source_offer(Wl->DataSource, "STRING");
	wl_data_source_offer(Wl->DataSource, "TEXT");
	// the compositor only accepts a selection with the serial of a recent user input event
	wl_data_device_set_selection(Wl->DataDevice, Wl->DataSource, Wl->InputSerial);
	wl_display_flush(Wl->Display);
}


//! Remove all messages pending in the system message loop
void CIrrDeviceLinux::clearSystemMessages()
{
	if (CreationParams.DriverType != video::EDT_NULL && Wl && Wl->Display)
	{
		Wl->DiscardInput = true;
		Wl->dispatch(0);
		Wl->DiscardInput = false;
		Wl->Repeating = false;
	}
}


// ----------------------------------------------------------------------
// cursor control
// ----------------------------------------------------------------------

CIrrDeviceLinux::CCursorControl::CCursorControl(CIrrDeviceLinux* dev, bool null)
	: Device(dev), IsVisible(true), Null(null), UseReferenceRect(false)
	, ActiveIcon(gui::ECI_NORMAL), ActiveIconStartTime(0), LastFrame(0)
{
	if (!Null)
		initCursors();
}

CIrrDeviceLinux::CCursorControl::~CCursorControl()
{
	// Do not clearCursors here as the display is already closed
}

bool CIrrDeviceLinux::CCursorControl::loadThemeCursor(const char* name)
{
	SCursor cursor;
	if (Device->Wl->CursorTheme)
	{
		struct wl_cursor* wc = wl_cursor_theme_get_cursor(Device->Wl->CursorTheme, name);
		if (wc && wc->image_count > 0)
		{
			SCursorFrame frame;
			struct wl_cursor_image* img = wc->images[0];
			frame.Buffer = wl_cursor_image_get_buffer(img);
			frame.Width = (s32)img->width;
			frame.Height = (s32)img->height;
			frame.HotX = (s32)img->hotspot_x;
			frame.HotY = (s32)img->hotspot_y;
			cursor.Frames.push_back(frame);
		}
	}
	const bool found = cursor.Frames.size() > 0;
	Cursors.push_back(cursor);
	return found;
}

void CIrrDeviceLinux::CCursorControl::initCursors()
{
	// Order of gui::ECURSOR_ICON. The names are those of the common cursor themes.
	static const char* const names[][2] = {
		{"left_ptr", "arrow"},
		{"crosshair", "cross"},
		{"hand2", "pointer"},
		{"question_arrow", "help"},
		{"xterm", "text"},
		{"crossed_circle", "X_cursor"},
		{"watch", "wait"},
		{"fleur", "all-scroll"},
		{"size_bdiag", "top_right_corner"},
		{"size_fdiag", "top_left_corner"},
		{"sb_v_double_arrow", "size_ver"},
		{"sb_h_double_arrow", "size_hor"},
		{"sb_up_arrow", "center_ptr"}
	};
	for (u32 i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
	{
		if (!loadThemeCursor(names[i][0]))
		{
			// the entry was added by loadThemeCursor, try the alternative name
			Cursors.erase(Cursors.size() - 1);
			loadThemeCursor(names[i][1]);
		}
	}
}

void CIrrDeviceLinux::CCursorControl::showFrame(const SCursorFrame& frame)
{
	SWayland* wl = Device->Wl;
	if (!wl || !wl->Pointer || !wl->CursorSurface || !wl->PointerInside || !frame.Buffer)
		return;
	wl_surface_attach(wl->CursorSurface, frame.Buffer, 0, 0);
	wl_surface_damage_buffer(wl->CursorSurface, 0, 0, frame.Width, frame.Height);
	wl_surface_commit(wl->CursorSurface);
	wl_pointer_set_cursor(wl->Pointer, wl->PointerEnterSerial, wl->CursorSurface, frame.HotX, frame.HotY);
}

void CIrrDeviceLinux::CCursorControl::applyCursor(unsigned int)
{
	SWayland* wl = Device->Wl;
	if (Null || !wl || !wl->Pointer || !wl->PointerInside)
		return;

	if (!IsVisible)
		wl_pointer_set_cursor(wl->Pointer, wl->PointerEnterSerial, NULL, 0, 0);
	else if ((u32)ActiveIcon < Cursors.size() && Cursors[ActiveIcon].Frames.size())
		showFrame(Cursors[ActiveIcon].Frames[0]);
}

void CIrrDeviceLinux::CCursorControl::setVisible(bool visible)
{
	if (visible == IsVisible)
		return;
	IsVisible = visible;
	if (!Null)
	{
		applyCursor(0);
		if (Device->Wl && Device->Wl->Display)
			wl_display_flush(Device->Wl->Display);
	}
}

void CIrrDeviceLinux::CCursorControl::update()
{
	if (Null || !IsVisible)
		return;
	if ( (u32)ActiveIcon < Cursors.size() && Cursors[ActiveIcon].Frames.size() > 1 && Cursors[ActiveIcon].FrameTime )
	{
		// update animated (custom) cursors
		u32 now = Device->getTimer()->getRealTime();
		u32 frame = ((now - ActiveIconStartTime) / Cursors[ActiveIcon].FrameTime) % Cursors[ActiveIcon].Frames.size();
		if (frame != LastFrame)
		{
			LastFrame = frame;
			showFrame(Cursors[ActiveIcon].Frames[frame]);
		}
	}
}

void CIrrDeviceLinux::CCursorControl::freeCursor(SCursor& cursor)
{
	for (u32 f = 0; f < cursor.Frames.size(); ++f)
	{
		if (cursor.Frames[f].Owned && cursor.Frames[f].Buffer)
			wl_buffer_destroy(cursor.Frames[f].Buffer);
	}
	cursor.Frames.clear();
}

void CIrrDeviceLinux::CCursorControl::clearCursors()
{
	for (u32 i = 0; i < Cursors.size(); ++i)
		freeCursor(Cursors[i]);
	Cursors.clear();
}

//! Sets the active cursor icon
void CIrrDeviceLinux::CCursorControl::setActiveIcon(gui::ECURSOR_ICON iconId)
{
	if (Null || iconId < 0 || iconId >= (s32)Cursors.size())
		return;

	ActiveIconStartTime = Device->getTimer()->getRealTime();
	ActiveIcon = iconId;
	LastFrame = 0;
	applyCursor(0);
	if (Device->Wl && Device->Wl->Display)
		wl_display_flush(Device->Wl->Display);
}

//! Creates a cursor from a sprite (premultiplied ARGB shared memory buffers as required by Wayland)
bool CIrrDeviceLinux::CCursorControl::createSpriteCursor(SCursor& cursor, const gui::SCursorSprite& icon)
{
	if (icon.SpriteId < 0 || !Device->Wl)
		return false;

	const gui::SGUISprite& sprite = icon.SpriteBank->getSprites()[icon.SpriteId];
	cursor.FrameTime = sprite.frameTime;
	for (u32 i = 0; i < sprite.Frames.size(); ++i)
	{
		const u32 texId = sprite.Frames[i].textureNumber;
		const u32 rectId = sprite.Frames[i].rectNumber;
		const core::rect<s32> rect = icon.SpriteBank->getPositions()[rectId];
		video::ITexture* tex = icon.SpriteBank->getTexture(texId);
		if (!tex || rect.getWidth() <= 0 || rect.getHeight() <= 0)
			continue;

		SShmBuffer shm;
		if (!SWayland::createShmBuffer(Device->Wl->Shm, (u32)rect.getWidth(), (u32)rect.getHeight(), WL_SHM_FORMAT_ARGB8888, shm))
			continue;

		const video::ECOLOR_FORMAT format = tex->getColorFormat();
		const u32 bytesPerPixel = video::IImage::getBitsPerPixelFromFormat(format) / 8;
		const u8* data = (const u8*)tex->lock(video::ETLM_READ_ONLY, 0);
		if (!data)
		{
			SWayland::destroyShmBuffer(shm);
			continue;
		}
		data += rect.UpperLeftCorner.Y * tex->getPitch() + rect.UpperLeftCorner.X * bytesPerPixel;
		u32* target = (u32*)shm.Data;
		for (s32 y = 0; y < rect.getHeight(); ++y)
		{
			const u8* p = data;
			for (s32 x = 0; x < rect.getWidth(); ++x)
			{
				video::SColor col;
				col.setData((const void*)p, format);
				p += bytesPerPixel;
				const u32 a = col.getAlpha();
				*target++ = (a << 24) | (((col.getRed() * a) / 255) << 16) | (((col.getGreen() * a) / 255) << 8) | ((col.getBlue() * a) / 255);
			}
			data += tex->getPitch();
		}
		tex->unlock();

		// the compositor keeps its own mapping, ours is not needed anymore
		munmap(shm.Data, shm.Size);

		SCursorFrame frame;
		frame.Buffer = shm.Buffer;
		frame.Width = rect.getWidth();
		frame.Height = rect.getHeight();
		frame.HotX = icon.HotSpot.X;
		frame.HotY = icon.HotSpot.Y;
		frame.Owned = true;
		cursor.Frames.push_back(frame);
	}
	return cursor.Frames.size() > 0;
}

//! Add a custom sprite as cursor icon.
gui::ECURSOR_ICON CIrrDeviceLinux::CCursorControl::addIcon(const gui::SCursorSprite& icon)
{
	if (Null)
		return gui::ECI_NORMAL;

	SCursor cursor;
	if (createSpriteCursor(cursor, icon))
	{
		Cursors.push_back(cursor);
		return (gui::ECURSOR_ICON)(Cursors.size() - 1);
	}
	return gui::ECI_NORMAL;
}

//! replace the given cursor icon.
void CIrrDeviceLinux::CCursorControl::changeIcon(gui::ECURSOR_ICON iconId, const gui::SCursorSprite& icon)
{
	if (Null || iconId < 0 || iconId >= (s32)Cursors.size())
		return;

	SCursor cursor;
	if (createSpriteCursor(cursor, icon))
	{
		freeCursor(Cursors[iconId]);
		Cursors[iconId] = cursor;
		if (iconId == ActiveIcon)
			applyCursor(0);
	}
}

core::dimension2di CIrrDeviceLinux::CCursorControl::getSupportedIconSize() const
{
	// Wayland has no hard limit, the cursor surface can be any size
	return core::dimension2di(64, 64);
}

} // end namespace

#endif // _IRR_COMPILE_WITH_WAYLAND_DEVICE_

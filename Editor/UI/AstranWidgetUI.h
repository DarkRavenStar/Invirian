#pragma once

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

namespace ImGui
{


	/*
	// Note: p_data, p_step, p_step_fast are _pointers_ to a memory address holding the data. For an Input widget, p_step and p_step_fast are optional.
	// Read code of e.g. InputFloat(), InputInt() etc. or examples in 'Demo->Widgets->Data Types' to understand how to use this function directly.
	bool ImGui::InputScalarCustom(const char* label, ImGuiDataType data_type, void* p_data, const void* p_step, const void* p_step_fast, const char* format, ImGuiInputTextFlags flags)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		ImGuiStyle& style = g.Style;

		if (format == NULL)
			format = DataTypeGetInfo(data_type)->PrintFmt;

		char buf[64];
		DataTypeFormatString(buf, IM_ARRAYSIZE(buf), data_type, p_data, format);

		// Testing ActiveId as a minor optimization as filtering is not needed until active
		if (g.ActiveId == 0 && (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsScientific)) == 0)
			flags |= InputScalar_DefaultCharsFilter(data_type, format);
		flags |= ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoMarkEdited; // We call MarkItemEdited() ourselves by comparing the actual data rather than the string.

		bool value_changed = false;
		if (p_step != NULL)
		{
			const float button_size = GetFrameHeight();

			BeginGroup(); // The only purpose of the group here is to allow the caller to query item data e.g. IsItemActive()
			PushID(label);
			SetNextItemWidth(ImMax(1.0f, CalcItemWidth() - (button_size + style.ItemInnerSpacing.x) * 2));
			if (InputText("", buf, IM_ARRAYSIZE(buf), flags)) // PushId(label) + "" gives us the expected ID from outside point of view
				value_changed = DataTypeApplyFromText(buf, data_type, p_data, format);

			// Step buttons
			const ImVec2 backup_frame_padding = style.FramePadding;
			style.FramePadding.x = style.FramePadding.y;
			ImGuiButtonFlags button_flags = ImGuiButtonFlags_Repeat | ImGuiButtonFlags_DontClosePopups;
			if (flags & ImGuiInputTextFlags_ReadOnly)
				BeginDisabled();
			SameLine(0, style.ItemInnerSpacing.x);
			if (ButtonEx("-", ImVec2(button_size, button_size), button_flags))
			{
				DataTypeApplyOp(data_type, '-', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
				value_changed = true;
			}
			SameLine(0, style.ItemInnerSpacing.x);
			if (ButtonEx("+", ImVec2(button_size, button_size), button_flags))
			{
				DataTypeApplyOp(data_type, '+', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
				value_changed = true;
			}
			if (flags & ImGuiInputTextFlags_ReadOnly)
				EndDisabled();

			const char* label_end = FindRenderedTextEnd(label);
			if (label != label_end)
			{
				SameLine(0, style.ItemInnerSpacing.x);
				TextEx(label, label_end);
			}
			style.FramePadding = backup_frame_padding;

			PopID();
			EndGroup();
		}
		else
		{
			if (InputText(label, buf, IM_ARRAYSIZE(buf), flags))
				value_changed = DataTypeApplyFromText(buf, data_type, p_data, format);
		}
		if (value_changed)
			MarkItemEdited(g.LastItemData.ID);

		return value_changed;
	}
*/

	

	void ToggleButton(const char* str_id, bool* v)
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		float height = ImGui::GetFrameHeight();
		float width = height * 1.55f;
		float radius = height * 0.50f;

		ImGui::InvisibleButton(str_id, ImVec2(width, height));
		if (ImGui::IsItemClicked()) *v = !*v;
		ImGuiContext& gg = *GImGui;
		float ANIM_SPEED = 0.085f;
		if (gg.LastActiveId == gg.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
			float t_anim = ImSaturate(gg.LastActiveIdTimer / ANIM_SPEED);
		if (ImGui::IsItemHovered())
			draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_ButtonActive] : ImVec4(0.78f, 0.78f, 0.78f, 1.0f)), height * 0.5f);
		else
			draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(*v ? colors[ImGuiCol_Button] : ImVec4(0.85f, 0.85f, 0.85f, 1.0f)), height * 0.50f);
		draw_list->AddCircleFilled(ImVec2(p.x + radius + (*v ? 1 : 0) * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
	}

	void ToggleButtonNoHover(const char* str_id, bool* v, float lineThicknessPercentage = 1.0f, float circleRadiusPercentage = 1.0f)
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		float height = ImGui::GetFrameHeight();
		float width = height * 1.55f;
		float radius = height * 0.50f;

		float lineThickness = radius * lineThicknessPercentage;
		float circleRadius = radius * circleRadiusPercentage;

		float offsetx = circleRadius;

		ImGui::InvisibleButton(str_id, ImVec2(width, height));
		if (ImGui::IsItemClicked()) *v = !*v;
		ImGuiContext& gg = *GImGui;

		auto buttonColor = ImGui::GetColorU32(*v ? colors[ImGuiCol_ButtonActive] : colors[ImGuiCol_Button]);

		draw_list->AddRectFilled(ImVec2(p.x, p.y + radius - lineThickness),
								 ImVec2(p.x + width, p.y + radius + lineThickness),
								 buttonColor, height * 0.50f);

		draw_list->AddCircleFilled(ImVec2(p.x + circleRadius + (*v ? 1 : 0) * (width - circleRadius * 2.0f), p.y + radius),
								   circleRadius,
								   IM_COL32(255, 255, 255, 255));
	}

	void PopAllStyleVar()
	{
		ImGuiContext& g = *GImGui;
		//ImGuiStackSizes* stack_sizes = &g.CurrentWindowStack.back().StackSizesOnBegin;
		PopStyleVar(g.StyleVarStack.Size);
	}

	void PopAllStyleColorVar()
	{
		ImGuiContext& g = *GImGui;
		PopStyleColor(g.ColorStack.Size);
	}

	// Button to minimize, maximize, close a window - mode: 0, 1, 2
	bool WindowButton(ImGuiID id, int mode)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		const float default_size = GetFrameHeight();
		ImVec2 size = ImVec2(g.FontSize, g.FontSize) + g.Style.FramePadding * 2.0f;

		// Tweak 1: Shrink hit-testing area if button covers an abnormally large proportion of the visible region. That's in order to facilitate moving the window away. (#3825)
		// This may better be applied as a general hit-rect reduction mechanism for all widgets to ensure the area to move window is always accessible?
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
		ImRect bb_interact = bb;
		const float area_to_visible_ratio = window->OuterRectClipped.GetArea() / bb.GetArea();
		if (area_to_visible_ratio < 1.5f)
			bb_interact.Expand(ImFloor(bb_interact.GetSize() * -0.25f));

		ItemSize(size, (size.y >= default_size) ? g.Style.FramePadding.y : -1.0f);

		// Tweak 2: We intentionally allow interaction when clipped so that a mechanical Alt,Right,Activate sequence can always close a window.
		// (this isn't the regular behavior of buttons, but it doesn't affect the user much because navigation tends to keep items visible).
		bool is_clipped = !ItemAdd(bb_interact, id);

		bool hovered, held;
		bool pressed = ButtonBehavior(bb_interact, id, &hovered, &held);
		if (is_clipped)
			return pressed;

		// Render
		// FIXME: Clarify this mess
		ImU32 col = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
		ImVec2 center = bb.GetCenter();
		if (hovered)
		{
			//RenderNavHighlight(bb, id);
			window->DrawList->AddRectFilled(bb.Min, bb.Max, col, 0);
			//window->DrawList->AddCircleFilled(center, ImMax(2.0f, g.FontSize * 0.5f + 1.0f), col, 12);
		}

		float cross_extent = g.FontSize * 0.5f * 0.7071f - 1.0f;
		ImU32 cross_col = GetColorU32(ImGuiCol_Text);
		center -= ImVec2(0.5f, 0.5f);

		if (mode == 0)
		{
			window->DrawList->AddLine(center + ImVec2(+cross_extent, 0), center + ImVec2(-cross_extent, 0), cross_col, 2.0f);
		}
		else if (mode == 1)
		{
			window->DrawList->AddLine(center + ImVec2(+cross_extent, +cross_extent), center + ImVec2(+cross_extent, -cross_extent), cross_col, 2.0f);
			window->DrawList->AddLine(center + ImVec2(+cross_extent, -cross_extent), center + ImVec2(-cross_extent, -cross_extent), cross_col, 2.0f);
			window->DrawList->AddLine(center + ImVec2(-cross_extent, -cross_extent), center + ImVec2(-cross_extent, +cross_extent), cross_col, 2.0f);
			window->DrawList->AddLine(center + ImVec2(-cross_extent, +cross_extent), center + ImVec2(+cross_extent, +cross_extent), cross_col, 2.0f);
		}
		else if (mode == 2)
		{
			window->DrawList->AddLine(center + ImVec2(+cross_extent, +cross_extent), center + ImVec2(-cross_extent, -cross_extent), cross_col, 2.0f);
			window->DrawList->AddLine(center + ImVec2(+cross_extent, -cross_extent), center + ImVec2(-cross_extent, +cross_extent), cross_col, 2.0f);
		}

		return pressed;
	}

	// Important: calling order matters!
	// FIXME: Somehow overlapping with docking tech.
	// FIXME: The "rect-cut" aspect of this could be formalized into a lower-level helper (rect-cut: https://halt.software/dead-simple-layouts)
	bool BeginViewportSideBarCustom(const char* name, ImGuiWindow* window, ImGuiDir dir, float axis_size, ImGuiWindowFlags window_flags)
	{
		/*
		IM_ASSERT(dir != ImGuiDir_None);

		ImGuiWindow* bar_window = FindWindowByName(name);
		ImGuiWindow* windowRef = window;
		if (bar_window == NULL || bar_window->BeginCount == 0)
		{
			// Calculate and set window size/position
			ImRect avail_rect = windowRef->GetBuildWorkRect();
			ImGuiAxis axis = (dir == ImGuiDir_Up || dir == ImGuiDir_Down) ? ImGuiAxis_Y : ImGuiAxis_X;
			ImVec2 pos = avail_rect.Min;
			if (dir == ImGuiDir_Right || dir == ImGuiDir_Down)
				pos[axis] = avail_rect.Max[axis] - axis_size;
			ImVec2 size = avail_rect.GetSize();
			size[axis] = axis_size;
			SetNextWindowPos(pos);
			SetNextWindowSize(size);

			// Report our size into work area (for next frame) using actual window size
			if (dir == ImGuiDir_Up || dir == ImGuiDir_Left)
				windowRef->BuildWorkOffsetMin[axis] += axis_size;
			else if (dir == ImGuiDir_Down || dir == ImGuiDir_Right)
				windowRef->BuildWorkOffsetMax[axis] -= axis_size;
		}

		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
		SetNextWindowViewport(windowRef->ID); // Enforce viewport so we don't create our own viewport when ImGuiConfigFlags_ViewportsNoMerge is set.
		PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); // Lift normal size constraint
		bool is_open = Begin(name, NULL, window_flags);
		PopStyleVar(2);

		return is_open;
		*/
		return true;
	}

	void CenteredSeparator(float width = 0)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;
		ImGuiContext& g = *GImGui;
		/*
		// Commented out because it is not tested, but it should work, but it won't be centered
		ImGuiWindowFlags flags = 0;
		if ((flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical)) == 0)
			flags |= (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;
		IM_ASSERT(ImIsPowerOfTwo((int)(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical))));   // Check that only 1 option is selected
		if (flags & ImGuiSeparatorFlags_Vertical)
		{
			VerticalSeparator();
			return;
		}
		*/

		// Horizontal Separator
		float x1, x2;
		//if (window->DC.ColumnsSet == NULL && (width == 0))
		if (window->DC.CurrentColumns == NULL && (width == 0))
		{
			// Span whole window
			///x1 = window->Pos.x; // This fails with SameLine(); CenteredSeparator();
			// Nah, we have to detect if we have a sameline in a different way
			x1 = window->DC.CursorPos.x;
			x2 = x1 + window->Size.x;
		}
		else
		{
			// Start at the cursor
			x1 = window->DC.CursorPos.x;
			if (width != 0)
			{
				x2 = x1 + width;
			}
			else
			{
				x2 = window->ClipRect.Max.x;
				// Pad right side of columns (except the last one)
				//if (window->DC.ColumnsSet && (window->DC.ColumnsSet->Current < window->DC.ColumnsSet->Count - 1))
				if (window->DC.CurrentColumns && (window->DC.CurrentColumns->Current < window->DC.CurrentColumns->Count - 1))
					x2 -= g.Style.ItemSpacing.x;
			}
		}
		//float y1 = window->DC.CursorPos.y + int(window->DC.CurrentLineHeight / 2.0f);
		float y1 = window->DC.CursorPos.y + int(window->DC.CurrLineSize.y / 2.0f);
		float y2 = y1 + 1.0f;

		window->DC.CursorPos.x += width; //+ g.Style.ItemSpacing.x;

		//if (!window->DC.GroupStack.empty()) x1 += window->DC.IndentX;
		if (!g.GroupStack.empty()) x1 += window->DC.Indent.x;

		const ImRect bb(ImVec2(x1, y1), ImVec2(x2, y2));
		ItemSize(ImVec2(0.0f, 0.0f)); // NB: we don't provide our width so that it doesn't get feed back into AutoFit, we don't provide height to not alter layout.
		if (!ItemAdd(bb, NULL))
		{
			return;
		}

		window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), GetColorU32(ImGuiCol_Border));

		/* // Commented out because LogText is hard to reach outside imgui.cpp
		if (g.LogEnabled)
		LogText(IM_NEWLINE "--------------------------------");
		*/
	}

	// Create a centered separator right after the current item.
	// Eg.: 
	// ImGui::PreSeparator(10);
	// ImGui::Text("Section VI");
	// ImGui::SameLineSeparator();
	void SameLineSeparator(float width = 0)
	{
		ImGui::SameLine();
		CenteredSeparator(width);
	}

	// Create a centered separator which can be immediately followed by a item
	void PreSeparator(float width)
	{
		ImGuiWindow* window = GetCurrentWindow();
		//if (window->DC.CurrentLineHeight == 0) window->DC.CurrentLineHeight = ImGui::GetTextLineHeight();
		if (window->DC.CurrLineSize.y == 0) window->DC.CurrLineSize.y = ImGui::GetTextLineHeight();
		CenteredSeparator(width);
		ImGui::SameLine();
	}

	// The value for width is arbitrary. But it looks nice.
	void TextSeparator(const char* text, float pre_width = 10.0f)
	{
		ImGui::PreSeparator(pre_width);
		ImGui::Text(text);
		ImGui::SameLineSeparator();
	}

	void test_fancy_separator()
	{
		ImGuiIO io = ImGui::GetIO();
		static float t = 0.0f;
		t += io.DeltaTime;
		float f = sinf(4 * t * 3.14f / 9.0f) * sinf(4 * t * 3.14f / 7.0f);
		ImGui::PreSeparator(20 + 100 * abs(f));
		ImGui::TextColored(ImColor(0.6f, 0.3f, 0.3f, 1.0f), "Fancy separators");
		ImGui::SameLineSeparator();
		ImGui::Bullet();
		ImGui::CenteredSeparator(100);
		ImGui::SameLine();
		ImGui::Text("Centered separator");
		ImGui::Columns(2);
		ImGui::PreSeparator(10);
		ImGui::Text("Separator");
		ImGui::SameLineSeparator();
		ImGui::CenteredSeparator();
		ImGui::Text("Column 1");
		ImGui::SameLineSeparator();

		ImGui::NextColumn();

		ImGui::PreSeparator(10);
		ImGui::Text("The Same Separator");
		ImGui::SameLineSeparator();
		ImGui::CenteredSeparator();
		ImGui::Text("Column 2");
		ImGui::SameLineSeparator();

		ImGui::Columns(1);
		ImGui::TextSeparator("So decorative");
		ImGui::CenteredSeparator();
	}
}

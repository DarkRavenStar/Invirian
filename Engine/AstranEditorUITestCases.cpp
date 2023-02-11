// void PropertyTableUI(const char* tableName)
// {
// 	if (doOnce)
// 	{
// 		ScopedImGuiColorAndStyle a(0, []()
// 		{
// 			ImGui::PushStyleColor(ImGuiCol_Button, black25);
// 		ImGui::PushStyleColor(ImGuiCol_Button, black25);
// 		ImGui::PushStyleColor(ImGuiCol_Button, black25);
// 		ImGui::PushStyleColor(ImGuiCol_Button, black25);
// 
// 		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 35.0f);
// 		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 35.0f);
// 		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 35.0f);
// 		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 35.0f);
// 		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 35.0f);
// 		});
// 
// 
// 
// 		{
// 
// 			ScopedImGuiColorAndStyle b(1, []()
// 			{
// 				ImGui::PushStyleColor(ImGuiCol_Button, black25);
// 
// 			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 35.0f);
// 			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 35.0f);
// 			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 35.0f);
// 			});
// 
// 		}
// 
// 		doOnce = false;
// 	}
// 
// 	ImGuiTableFlags tableFlags =
// 	{
// 		ImGuiTableFlags_SizingFixedFit
// 		| ImGuiTableFlags_Resizable
// 		| ImGuiTableFlags_NoSavedSettings
// 		| ImGuiTableFlags_Borders
// 		//| ImGuiTableFlags_BordersInnerH
// 		//| ImGuiTableFlags_BordersOuterH
// 		//| ImGuiTableFlags_BordersInnerV
// 		//| ImGuiTableFlags_BordersOuterV
// 	};
// 
// 
// 	//ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(1, 1));
// 	//ImGui::PopStyleVar(1);
// 	if (ImGui::BeginTable(tableName, 2, tableFlags))
// 	{
// 		ImGui::TableSetupColumn("PropertyName", ImGuiTableColumnFlags_WidthStretch);
// 		ImGui::TableSetupColumn("PropertyValue", ImGuiTableColumnFlags_WidthStretch);
// 		//ImGui::TableSetColumnWidth()
// 		//ImGui::TableNextRow();
// 		ImGui::TableNextColumn();
// 		ImGui::AlignTextToFramePadding();
// 		ImGui::Text("Combo");
// 		ImGui::TableNextColumn();
// 		ComboBoxUI("Combo");
// 
// 		ImGui::EndTable();
// 	}
// }
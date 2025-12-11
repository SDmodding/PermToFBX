#pragma once
#define BYTE4N_FLT(x) ((static_cast<f32>(x) / 255.f) * 2.f - 1.f)

namespace core
{
	using namespace UFG;

	const char* GetParamValue(const char* arg, const qString& param)
	{
		if (const char* find = qStringFindInsensitive(arg, param)) {
			return &find[param.Length()];
		}

		return 0;
	}

	bool IsPermFile(const qString& path)
	{
		qString path2 = path.ToLower();
		return (path2.EndsWith(".bin") && !path2.EndsWith(".temp.bin"));
	}

	void LoadPermFiles(const qString& find_path)
	{
		WIN32_FIND_DATAA wFindData = { 0 };
		HANDLE hFind = FindFirstFileA(find_path, &wFindData);

		if (hFind == INVALID_HANDLE_VALUE) {
			return;
		}

		char* end = &find_path.mData[find_path.mLength - 2];
		bool recursive = (end[0] == '*' && end[1] == '*');

		qString folder = find_path.GetFilePath() + "\\";

		do
		{
			qString file_path = (folder + wFindData.cFileName);

			if (wFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (wFindData.cFileName[0] == '.') {
					continue;
				}

				if (recursive)
				{
					file_path += "\\**";
					LoadPermFiles(file_path);
				}

				continue;
			}

			if (IsPermFile(file_path)) {
				StreamResourceLoader::LoadResourceFile(file_path);
			}
		} while (FindNextFileA(hFind, &wFindData));

		FindClose(hFind);
	}

	Illusion::VertexStreamDescriptor* GetVertexStreamDescriptor(u32 name_uid)
	{
		auto streamDescriptors = Illusion::VertexStreamDescriptor::GetStreamDescriptors();
		for (auto streamDescriptor = streamDescriptors->begin(); streamDescriptor != streamDescriptors->end(); streamDescriptor = streamDescriptor->next())
		{
			if (streamDescriptor->mNameUID == name_uid) {
				return streamDescriptor;
			}
		}

		return 0;
	}

	Illusion::VertexStreamElement* GetVertexStreamElement(Illusion::VertexStreamDescriptor* stream_descriptor, Illusion::VertexStreamElementUsage usage)
	{
		for (int i = 0; stream_descriptor->GetTotalElements() > i; ++i)
		{
			auto stream_element = stream_descriptor->GetElement(i);
			if (stream_element && stream_element->mUsage == usage) {
				return stream_element;
			}
		}

		return 0;
	}

	void* GetVertexStreamData(Illusion::VertexStreamDescriptor* stream_descriptor, Illusion::VertexStreamElement* stream_element, Illusion::Buffer* vertex_buffer, u32 index)
	{
		return vertex_buffer->mData.Get(stream_element->mOffset + (stream_descriptor->GetStreamSize(stream_element->mStream) * index));
	}

	void InitMeshHandles(Illusion::Mesh* mesh)
	{
		auto warehouse = qResourceWarehouse::Instance();

		mesh->mMaterialHandle.mData = warehouse->DebugGet(RTypeUID_Material, mesh->mMaterialHandle.mNameUID);
		mesh->mIndexBufferHandle.mData = warehouse->DebugGet(RTypeUID_Buffer, mesh->mIndexBufferHandle.mNameUID);

		for (auto& vertexBufferHandle : mesh->mVertexBufferHandles) {
			vertexBufferHandle.mData = warehouse->DebugGet(RTypeUID_Buffer, vertexBufferHandle.mNameUID);
		}
	}

	bool ValidateBonesWithRig(RigResource* rig, Illusion::BonePalette* bone_palette)
	{
		auto skeleton = rig->mSkeleton;

		for (u32 b = 0; bone_palette->mNumBones > b; ++b)
		{
			bool found = 0;
			u32 bone_uid = *bone_palette->mBoneUIDTable[b];

			for (int i = 0; skeleton->m_bones.m_size > i; ++i)
			{
				auto bone = &skeleton->m_bones.m_data[i];
				if (qStringHashUpper32(bone->m_name) == bone_uid)
				{
					found = 1;
					break;
				}
			}

			if (!found) {
				return 0;
			}
		}

		return 1;
	}
}
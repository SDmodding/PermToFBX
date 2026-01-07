#define THEORY_IMPL
#define THEORY_DUCKTAPE
#define THEORY_SLIM_BUILD
#define THEORY_IMPL_VERTEX_FORMAT
#include "theory/theory.hh"

using namespace UFG;

#include "core.hh"
#include "texmgr.hh"

//--------------------------------------------------
//	FBX SDK
//--------------------------------------------------

#include <fbxsdk.h> // 2020.3.7
#ifdef _DEBUG
	#pragma comment(lib, "libfbxsdk-md.lib")
	#pragma comment(lib, "libxml2-md.lib")
	#pragma comment(lib, "zlib-md.lib")
#else
	#pragma comment(lib, "libfbxsdk-mt.lib")
	#pragma comment(lib, "libxml2-mt.lib")
	#pragma comment(lib, "zlib-mt.lib")
#endif

//--------------------------------------------------
//	FBX Model
//--------------------------------------------------

#include "fbxmodel.hh"

//--------------------------------------------------
//	Inventories
//--------------------------------------------------

qResourceInventory gMaterialInventory = { "MaterialInventory", RTypeUID_Material, ChunkUID_Material };
qResourceInventory gModelInventory = { "ModelInventory", RTypeUID_Model, ChunkUID_Model };
qResourceInventory gTextureInventory = { "TextureInventory", RTypeUID_Texture, ChunkUID_Texture };
qResourceInventory gBufferInventory = { "BufferInventory", RTypeUID_Buffer, ChunkUID_Buffer };
qResourceInventory gBonePaletteInventory = { "BonePaletteInventory", RTypeUID_BonePalette, ChunkUID_BonePalette };
qResourceInventory gRigResourceInventory = { "RigResourceInventory", RTypeUID_RigResource, ChunkUID_RigResource };

//--------------------------------------------------
//	Export Logic
//--------------------------------------------------

void ExportModel(const char* output_path, fbxsdk::FbxManager* mgr, Illusion::Model* mdl, UFG::RigResource* rig)
{
	qPrintf("[ INFO ] Exporting: %s\n", mdl->mDebugName);

	auto warehouse = qResourceWarehouse::Instance();

	auto fbxModel = qFBXModel(mgr);
	fbxsdk::FbxSkin* fbxSkin = 0;
	fbxsdk::FbxArray<fbxsdk::FbxNode*> fbxBoneNodes;

	auto bonePalette = static_cast<Illusion::BonePalette*>(warehouse->DebugGet(RTypeUID_BonePalette, mdl->mBonePaletteHandle.mNameUID));
	int num_bones = 0;

	if (bonePalette && rig)
	{
		auto skeleton = rig->mSkeleton;
		num_bones = skeleton->m_bones.m_size;

		if (bonePalette->mNumBones > static_cast<u32>(num_bones)) {
			qPrintf("[ WARN ] %s (u) has more bones than skeleton in %s (i)\n", bonePalette->mDebugName, bonePalette->mNumBones, rig->mDebugName, num_bones);
		}
		else if (!core::ValidateBonesWithRig(rig, bonePalette)) {
			qPrintf("[ WARN ] %s doesn't match skeleton bones in %s\n", bonePalette->mDebugName, rig->mDebugName);
		}
		else
		{
			for (int i = 0; num_bones > i; ++i)
			{
				auto bone = &skeleton->m_bones.m_data[i];
				auto trans = &skeleton->m_referencePose.m_data[i];

				fbxsdk::FbxAMatrix matrix;

				matrix.SetT(fbxsdk::FbxVector4(
					static_cast<double>(trans->m_translation.m_quad.m128_f32[0]),
					static_cast<double>(trans->m_translation.m_quad.m128_f32[1]),
					static_cast<double>(trans->m_translation.m_quad.m128_f32[2])
				));

				matrix.SetQ(fbxsdk::FbxQuaternion(
					static_cast<double>(trans->m_rotation.m_vec.m_quad.m128_f32[0]),
					static_cast<double>(trans->m_rotation.m_vec.m_quad.m128_f32[1]),
					static_cast<double>(trans->m_rotation.m_vec.m_quad.m128_f32[2]),
					static_cast<double>(trans->m_rotation.m_vec.m_quad.m128_f32[3])
				));

				matrix.SetS(fbxsdk::FbxVector4(
					static_cast<double>(trans->m_scale.m_quad.m128_f32[0]),
					static_cast<double>(trans->m_scale.m_quad.m128_f32[1]),
					static_cast<double>(trans->m_scale.m_quad.m128_f32[2])
				));

				fbxBoneNodes.Add(fbxModel.CreateLimbNode(bone->m_name, matrix));
			}

			for (int i = 0; num_bones > i; ++i)
			{
				int parent = skeleton->m_parentIndices.m_data[i];

				if (parent == -1) {
					fbxModel.mScene->GetRootNode()->AddChild(fbxBoneNodes[i]);
				}
				else {
					fbxBoneNodes[parent]->AddChild(fbxBoneNodes[i]);
				}
			}

			fbxSkin = fbxModel.CreateSkin(bonePalette->mDebugName);
		}
	}

	for (u32 m = 0; mdl->mNumMeshes > m; ++m)
	{
		auto mesh = mdl->GetMesh(m);
		core::InitMeshHandles(mesh);

		auto qPrintPrefix = [&](const char* prefix, const char* str) { qPrintf("[ %s ] Mesh %s (Index %u) %s", prefix, mdl->mDebugName, m, str); };

		auto vertexStreamDesc = core::GetVertexStreamDescriptor(mesh->mVertexDeclHandle.mNameUID);
		if (!vertexStreamDesc)
		{
			qPrintPrefix("ERROR", " missing vertex stream descriptor!\n");
			continue;
		}

		auto material = mesh->mMaterialHandle.GetData();
		if (!material)
		{
			qPrintPrefix("ERROR", " missing material!\n");
			continue;
		}

		auto indexBuffer = mesh->mIndexBufferHandle.GetData();
		if (!indexBuffer)
		{
			qPrintPrefix("ERROR", " missing index buffer!\n");
			continue;
		}

		auto vertex_position_stream = core::GetVertexStreamElement(vertexStreamDesc, Illusion::VERTEX_ELEMENT_POSITION);
		auto vertexBuffer = mesh->mVertexBufferHandles[vertex_position_stream->mStream].GetData();
		if (!vertexBuffer)
		{
			qPrintPrefix("ERROR", " missing vertex buffer!\n");
			continue;
		}

		qString meshName = { "%s.%u", mdl->mDebugName, m };
		auto fbxMesh = fbxModel.CreateMesh(meshName, vertexBuffer->mNumElements);
		auto fbxNode = fbxMesh->GetNode();

		auto fbxUV = fbxMesh->CreateElementUV("UV0");
		fbxUV->SetMappingMode(fbxsdk::FbxGeometryElement::eByPolygonVertex);
		fbxUV->SetReferenceMode(fbxsdk::FbxGeometryElement::eIndexToDirect);

		// Material
		for (u32 p = 0; material->mNumParams > p; ++p)
		{
			auto param = material->GetParam(p);
			if (param->mNameUID == 0xDCE06689)
			{
				qString filename = TextureManager::FindTextureFile(output_path, param->mResourceHandle.mNameUID);
				auto fbxMat = fbxModel.CreateDiffuseMaterial(material->mDebugName, filename, fbxUV->GetName());
				fbxNode->AddMaterial(fbxMat);
			}
			else if (param->mNameUID == 0xADBE1A5A)
			{
				qString filename = TextureManager::FindTextureFile(output_path, param->mResourceHandle.mNameUID);
				auto fbxMat = fbxModel.CreateBumpMaterial(material->mDebugName, filename, fbxUV->GetName());
				fbxNode->AddMaterial(fbxMat);
			}
		}

		// Positions
		{
			auto stream_element = vertex_position_stream;

			auto cp = fbxMesh->GetControlPoints();
			for (u32 v = 0; vertexBuffer->mNumElements > v; ++v)
			{
				auto pos = static_cast<f32*>(core::GetVertexStreamData(vertexStreamDesc, stream_element, vertexBuffer, v));
				cp[v] = fbxsdk::FbxVector4(static_cast<double>(pos[0]), static_cast<double>(pos[1]), static_cast<double>(pos[2]));
			}
		}

		// Normals

		if (auto stream_element = core::GetVertexStreamElement(vertexStreamDesc, Illusion::VERTEX_ELEMENT_NORMAL))
		{
			auto fbxNormal = fbxMesh->CreateElementNormal();
			fbxNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
			fbxNormal->SetReferenceMode(FbxGeometryElement::eDirect);

			auto normalBuffer = mesh->mVertexBufferHandles[stream_element->mStream].GetData();

			for (u32 v = 0; normalBuffer->mNumElements > v; ++v)
			{
				auto data = core::GetVertexStreamData(vertexStreamDesc, stream_element, normalBuffer, v);
				fbxsdk::FbxVector4 normal = { 0.0, 0.0, 0.0, 1.0 };

				for (int i = 0; 3 > i; ++i)
				{
					switch (stream_element->mType)
					{
					case Illusion::VERTEX_TYPE_FLOAT3:
					case Illusion::VERTEX_TYPE_FLOAT4:
						normal[i] = static_cast<double>(static_cast<f32*>(data)[i]); break;
					case Illusion::VERTEX_TYPE_BYTE4N:
						normal[i] = static_cast<double>(BYTE4N_FLT(static_cast<u8*>(data)[i])); break;
					}
					
				}

				fbxNormal->GetDirectArray().Add(normal);
			}
		}

		// Texture Coord

		if (auto stream_element = core::GetVertexStreamElement(vertexStreamDesc, Illusion::VERTEX_ELEMENT_TEXCOORD0))
		{
			auto uvBuffer = mesh->mVertexBufferHandles[stream_element->mStream].GetData();

			for (u32 v = 0; uvBuffer->mNumElements > v; ++v)
			{
				auto data = core::GetVertexStreamData(vertexStreamDesc, stream_element, uvBuffer, v);
				fbxsdk::FbxVector2 uv = { 0.0, 1.0 };

				for (int i = 0; 2 > i; ++i)
				{
					switch (stream_element->mType)
					{
					case Illusion::VERTEX_TYPE_HALF2:
						uv[i] = static_cast<double>(static_cast<qHalfFloat*>(data)[i].Get()); break;
					}
				}

				uv[1] = 1.0 - uv[1];

				fbxUV->GetDirectArray().Add(uv);
			}
		}

		// Polygons

		if (4 >= indexBuffer->mElementByteSize)
		{
			auto indices = static_cast<u8*>(indexBuffer->mData.Get(indexBuffer->mElementByteSize * mesh->mIndexStart));
			for (u32 p = 0; mesh->mNumPrims > p; ++p)
			{
				fbxMesh->BeginPolygon();

				for (int i = 0; 3 > i; ++i)
				{
					int index = 0;
					memcpy(&index, indices, indexBuffer->mElementByteSize);

					fbxMesh->AddPolygon(index);
					fbxUV->GetIndexArray().Add(index);

					indices = &indices[indexBuffer->mElementByteSize];
				}

				fbxMesh->EndPolygon();
			}
		}

		// Blend Indexes & Weights (Rig)

		auto index_element = core::GetVertexStreamElement(vertexStreamDesc, Illusion::VERTEX_ELEMENT_BLENDINDEX);
		auto weight_element = core::GetVertexStreamElement(vertexStreamDesc, Illusion::VERTEX_ELEMENT_BLENDWEIGHT);

		if (index_element && weight_element && fbxSkin)
		{
			fbxsdk::FbxArray<u32> boneNames;
			for (int i = 0; num_bones > i; ++i)
			{
				auto name = fbxBoneNodes[i]->GetName();
				boneNames.Add(qStringHashUpper32(name));
			}

			fbxsdk::FbxArray<fbxsdk::FbxCluster*> fbxClusters;
			auto& matrix = fbxNode->EvaluateGlobalTransform();

			for (u32 i = 0; bonePalette->mNumBones > i; ++i)
			{
				fbxsdk::FbxNode* node = 0;
				u32 bone_uid = *bonePalette->mBoneUIDTable[i];

				for (int i = 0; num_bones > i; ++i)
				{
					if (bone_uid == boneNames[i])
					{
						node = fbxBoneNodes[i];
						break;
					}
				}

				fbxClusters.Add(fbxModel.CreateCluster(fbxSkin, node, matrix));
			}

			if (fbxClusters.Size() > 0)
			{
				auto indexBuffer = mesh->mVertexBufferHandles[index_element->mStream].GetData();
				auto weightBuffer = mesh->mVertexBufferHandles[weight_element->mStream].GetData();

				for (u32 v = 0; weightBuffer->mNumElements > v; ++v)
				{
					auto indexes = static_cast<u8*>(core::GetVertexStreamData(vertexStreamDesc, index_element, indexBuffer, v));
					auto weights = static_cast<u8*>(core::GetVertexStreamData(vertexStreamDesc, weight_element, weightBuffer, v));

					for (int i = 0; 4 > i; ++i)
					{
						u8 bone_index = indexes[i];
						f32 weight = static_cast<f32>(weights[i]) / 255.f;

						auto cluster = fbxClusters[bone_index];
						if (!cluster) {
							continue;
						}

						cluster->AddControlPointIndex(v, weight);
					}
				}

				fbxMesh->AddDeformer(fbxSkin);
			}
		}
	}

	qString filename = { "%s\\%s.fbx", output_path, mdl->mDebugName };
	if (auto device = gQuarkFileSystem.MapFilenameToDevice(output_path)) {
		device->CreateDirectoryA(output_path);
	}
	fbxModel.Export(mgr, filename);
}

int main(int argc, char** argv)
{
	qInit(0);
	Illusion::gEngine.Init();

	if (1 >= argc)
	{
		qPrintf("ERROR: Missing arguments!\n");
		return 1;
	}

	qString output_path = "output";
	qString rig_name;
	qString model_name;

	// Handle Arguments

	for (int i = 1; argc > i; ++i)
	{
		qString arg = argv[i];

		if (core::IsPermFile(arg))
		{
			StreamResourceLoader::LoadResourceFile(arg);
			continue;
		}

		if (arg.EndsWith("\\*") || arg.EndsWith("/*") || arg.EndsWith("\\**") || arg.EndsWith("/**"))
		{
			core::LoadPermFiles(arg);
			continue;
		}

		if (auto param = core::GetParamValue(arg, "-output="))
		{
			output_path = param;
			continue;
		}

		if (auto param = core::GetParamValue(arg, "-rig="))
		{
			rig_name = param;
			continue;
		}

		if (auto param = core::GetParamValue(arg, "-model="))
		{
			model_name = param;
			continue;
		}
	}

	// Load Rig...

	RigResource* rig = 0;

	if (!rig_name.IsEmpty())
	{
		rig = static_cast<RigResource*>(gRigResourceInventory.Get(rig_name.GetStringHashUpper32()));
		if (!rig)
		{
			qPrintf("ERROR: Failed to find rig (%s)!\nDid you forgot to load file with the specific rig?\n", rig_name.mData);
			return 1;
		}

		rig->mSkeleton = static_cast<hkaSkeleton*>(NativePackfileUtils::loadInPlace(rig->GetHavokMemImagedData(), rig->mHavokMemImagedDataSize, 0));
	}

	// Handle exporting...

	if (gModelInventory.mResourceDatas.IsEmpty())
	{
		qPrintf("ERROR: No models has been loaded!\n");
		return 1;
	}

	auto sdkMgr = fbxsdk::FbxManager::Create();

	auto ios = fbxsdk::FbxIOSettings::Create(sdkMgr, IOSROOT);
	sdkMgr->SetIOSettings(ios);

	for (auto resource : gModelInventory.mResourceDatas)
	{
		auto mdl = static_cast<Illusion::Model*>(resource);

		if (!model_name.IsEmpty())
		{
			u32 model_nameuid = mdl->mNode.mUID;
			if (model_nameuid != model_name.GetStringHash32() && model_nameuid != model_name.GetStringHashUpper32() && qStringCompareInsensitive(mdl->mDebugName, model_name) != 0)
			{
				qPrintf("[ INFO ] Ignoring %s\n", mdl->mDebugName);
				continue;
			}
		}

		ExportModel(output_path, sdkMgr, mdl, rig);
	}

	qClose();

	return 0;
}
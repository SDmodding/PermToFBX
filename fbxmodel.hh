#pragma once

class qFBXModel
{
public:
	fbxsdk::FbxScene* mScene = 0;

	qFBXModel(fbxsdk::FbxManager* mgr)
	{
		mScene = fbxsdk::FbxScene::Create(mgr, "");

		mScene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);
	}

	~qFBXModel()
	{
		mScene->Destroy(1);
	}

	bool Export(fbxsdk::FbxManager* mgr, const char* filename)
	{
		fbxsdk::FbxAxisSystem targetSystem(fbxsdk::FbxAxisSystem::EUpVector::eYAxis, fbxsdk::FbxAxisSystem::EFrontVector::eParityOdd, fbxsdk::FbxAxisSystem::eRightHanded);
		targetSystem.DeepConvertScene(mScene);

		FbxExporter* exporter = FbxExporter::Create(mgr, "");

		bool exported = exporter->Initialize(filename, -1, mgr->GetIOSettings());
		if (!exporter->Export(mScene)) {
			exported = 0;
		}

		exporter->Destroy();
		return exported;
	}

	fbxsdk::FbxMesh* CreateMesh(const char* name, int num_cp)
	{
		auto node = fbxsdk::FbxNode::Create(mScene, name);
		auto mesh = fbxsdk::FbxMesh::Create(mScene, name);

		node->SetNodeAttribute(mesh);
		mesh->InitControlPoints(num_cp);

		mScene->GetRootNode()->AddChild(node);

		return mesh;
	}

	fbxsdk::FbxSurfacePhong* CreateDiffuseMaterial(const char* name, const char* filename, const char* uvset)
	{
		auto fileTexture = fbxsdk::FbxFileTexture::Create(mScene, name);
		fileTexture->SetFileName(filename);
		fileTexture->SetTextureUse(fbxsdk::FbxTexture::eStandard);
		fileTexture->SetMappingType(fbxsdk::FbxTexture::eUV);
		fileTexture->SetMaterialUse(fbxsdk::FbxFileTexture::eModelMaterial);
		fileTexture->SetSwapUV(0);
		fileTexture->UVSet.Set(uvset);

		auto material = fbxsdk::FbxSurfacePhong::Create(mScene, name);
		material->Diffuse.Set(fbxsdk::FbxDouble3(1, 1, 1));
		material->DiffuseFactor.Set(1.0);
		material->Diffuse.ConnectSrcObject(fileTexture);

		return material;
	}

	fbxsdk::FbxSurfacePhong* CreateBumpMaterial(const char* name, const char* filename, const char* uvset)
	{
		auto fileTexture = fbxsdk::FbxFileTexture::Create(mScene, name);
		fileTexture->SetFileName(filename);
		fileTexture->SetTextureUse(fbxsdk::FbxTexture::eBumpNormalMap);
		fileTexture->SetMappingType(fbxsdk::FbxTexture::eUV);
		fileTexture->SetMaterialUse(fbxsdk::FbxFileTexture::eModelMaterial);
		fileTexture->SetSwapUV(0);
		fileTexture->UVSet.Set(uvset);

		auto layeredTexture = fbxsdk::FbxLayeredTexture::Create(mScene, "");
		layeredTexture->SetTextureUse(fbxsdk::FbxLayeredTexture::eBumpNormalMap);
		layeredTexture->ConnectSrcObject(fileTexture);

		auto material = fbxsdk::FbxSurfacePhong::Create(mScene, name);
		material->Bump.Set(fbxsdk::FbxDouble3(1, 1, 1));
		material->Bump.ConnectSrcObject(fileTexture);

		return material;
	}

	fbxsdk::FbxSkin* CreateSkin(const char* name)
	{
		return fbxsdk::FbxSkin::Create(mScene, name);
	}

	fbxsdk::FbxNode* CreateLimbNode(const char* name, const fbxsdk::FbxAMatrix& matrix)
	{
		auto skel = fbxsdk::FbxSkeleton::Create(mScene, name);
		skel->SetSkeletonType(fbxsdk::FbxSkeleton::eLimbNode);

		auto node = fbxsdk::FbxNode::Create(mScene, name);
		node->SetNodeAttribute(skel);

		node->LclTranslation.Set(matrix.GetT());
		node->LclRotation.Set(matrix.GetR());
		node->LclScaling.Set(matrix.GetS());

		return node;
	}

	fbxsdk::FbxCluster* CreateCluster(fbxsdk::FbxSkin* skin, fbxsdk::FbxNode* node, const fbxsdk::FbxAMatrix& matrix)
	{
		auto cluster = fbxsdk::FbxCluster::Create(mScene, node->GetName());
		cluster->SetLink(node);
		cluster->SetLinkMode(fbxsdk::FbxCluster::eNormalize);

		cluster->SetTransformMatrix(matrix);
		cluster->SetTransformLinkMatrix(node->EvaluateGlobalTransform());

		skin->AddCluster(cluster);

		return cluster;
	}
};
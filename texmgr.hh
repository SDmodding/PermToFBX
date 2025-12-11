#pragma once

struct DDS_PIXELFORMAT
{
    u32 dwSize;
    u32 dwFlags;
    u32 dwFourCC;
    u32 dwRGBBitCount;
    u32 dwRBitMask;
    u32 dwGBitMask;
    u32 dwBBitMask;
    u32 dwABitMask;
};

struct DDS_HEADER
{
    u32 dwSize;
    u32 dwFlags;
    u32 dwHeight;
    u32 dwWidth;
    u32 dwPitchOrLinearSize;
    u32 dwDepth;
    u32 dwMipMapCount;
    u32 dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    u32 dwCaps;
    u32 dwCaps2;
    u32 dwCaps3;
    u32 dwCaps4;
    u32 dwReserved2;
};

using namespace UFG;

namespace TextureManager
{
    void ConvertToDDS(Illusion::Texture* texture, DDS_HEADER& dds)
    {
        auto& ddspf = dds.ddspf;

        dds.dwSize = sizeof(DDS_HEADER);
        dds.dwWidth = texture->mWidth;
        dds.dwHeight = texture->mHeight;
        dds.dwPitchOrLinearSize = ((dds.dwWidth + 3) / 4) * ((dds.dwHeight + 3) / 4);

        dds.dwFlags = 0;
        dds.dwFlags |= 0x1;     // DDSD_CAPS
        dds.dwFlags |= 0x2;     // DDSD_HEIGHT
        dds.dwFlags |= 0x4;     // DDSD_WIDTH
        dds.dwFlags |= 0x1000;  // DDSD_PIXELFORMAT
        dds.dwFlags |= 0x80000; // DDSD_LINEARSIZE

        dds.dwCaps = 0x1000; // DDSCAPS_TEXTURE

        if (texture->mNumMipMaps > 0)
        {
            dds.dwMipMapCount = texture->mNumMipMaps;

            dds.dwFlags |= 0x20000; // DDSD_MIPMAPCOUNT

            dds.dwCaps |= 0x8;      // DDSCAPS_COMPLEX
            dds.dwCaps |= 0x400000; // DDSCAPS_MIPMAP
        }

        ddspf.dwSize = 32;

        switch (texture->mFormat)
        {
        case Illusion::Texture::FORMAT_A8R8G8B8:
            ddspf.dwFlags |= 0x41; // DDPF_RGB | DDPF_ALPHAPIXELS
            break;
        case Illusion::Texture::FORMAT_X8:
        case Illusion::Texture::FORMAT_X16: // Not sure
            ddspf.dwFlags |= 0x40; // DDPF_RGB
            break;
        case Illusion::Texture::FORMAT_DXT1: case Illusion::Texture::FORMAT_DXT3: case Illusion::Texture::FORMAT_DXT5:
        case Illusion::Texture::FORMAT_DXN:
            ddspf.dwFlags |= 0x4; // DDPF_FOURCC
            break;
        }

        switch (texture->mFormat)
        {
        case Illusion::Texture::FORMAT_DXT1:
        {
            dds.dwPitchOrLinearSize *= 8;

            ddspf.dwFourCC = MAKEFOURCC('D','X','T','1');
        }
        break;
        case Illusion::Texture::FORMAT_DXT3:
        {
            dds.dwPitchOrLinearSize *= 16;

            ddspf.dwFourCC = MAKEFOURCC('D','X','T','3');
        }
        break;
        case Illusion::Texture::FORMAT_DXT5:
        {
            dds.dwPitchOrLinearSize *= 16;

            ddspf.dwFourCC = MAKEFOURCC('D','X','T','5');
        }
        break;
        case Illusion::Texture::FORMAT_DXN:
        {
            dds.dwPitchOrLinearSize *= 16;

            ddspf.dwFourCC = MAKEFOURCC('A','T','I','2');
        }
        break;
        }

        // RGBA Format

        switch (texture->mFormat)
        {
        case Illusion::Texture::FORMAT_A8R8G8B8:
        case Illusion::Texture::FORMAT_X8:
        case Illusion::Texture::FORMAT_X16: // Not sure
        {
            ddspf.dwRBitMask = 0x000000FF;
            ddspf.dwGBitMask = 0x0000FF00;
            ddspf.dwBBitMask = 0x00FF0000;
            ddspf.dwABitMask = 0xFF000000;
        }
        break;
        case Illusion::Texture::FORMAT_DXT1: case Illusion::Texture::FORMAT_DXT3: case Illusion::Texture::FORMAT_DXT5:
        case Illusion::Texture::FORMAT_DXN:
        {
            ddspf.dwRBitMask = 0xC5FBE8;
            ddspf.dwGBitMask = 0x10745D6;
            ddspf.dwBBitMask = 0x108C1C0;
            ddspf.dwABitMask = 0xC5FBF4;
        }
        break;
        default:
        {
            ddspf.dwRBitMask = 0x0;
            ddspf.dwGBitMask = 0x0;
            ddspf.dwBBitMask = 0x0;
            ddspf.dwABitMask = 0x0;
        }
        break;
        }
    }

    qString* GetFilenameToTexture(Illusion::Texture* data)
    {
        for (auto loaded_file : StreamResourceLoader::smLoadedFiles)
        {
            void* data_end = reinterpret_cast<void*>(reinterpret_cast<uptr>(loaded_file->mData) + loaded_file->mDataSize);
            if (data >= loaded_file->mData && data_end >= data) {
                return &loaded_file->mFilename;
            }
        }

        return 0;
    }

    qString GetFilenameToTextureData(Illusion::Texture* data)
    {
        auto filename = GetFilenameToTexture(data);
        if (!filename) {
            return "";
        }

        auto tempname = *filename;
        tempname.ReplaceString(".perm.bin", ".temp.bin", 1);
        return tempname;
    }

    void* GetTextureData(Illusion::Texture* texture)
    {
        auto filename = GetFilenameToTextureData(texture);
        if (filename.IsEmpty()) {
            return 0;
        }

        void* buffer = qMalloc(texture->mImageDataByteSize);
        if (buffer) 
        {
            auto file = qOpen(filename, QACCESS_READ);
            if (!file)
            {
                qFree(buffer);
                return 0;
            }

            qRead(file, buffer, texture->mImageDataByteSize, texture->mImageDataPosition, QSEEK_SET);
            qClose(file);
        }

        return buffer;
    }

    void ExportTexture(Illusion::Texture* texture, const char* filename)
    {
        auto file = qOpen(filename, QACCESS_WRITE);
        if (!file) {
            return;
        }

        u32 magic = 0x20534444;
        qWrite(file, &magic, sizeof(magic));

        DDS_HEADER dds;
        {
            qMemSet(&dds, 0, sizeof(dds));
            ConvertToDDS(texture, dds);
        }
        qWrite(file, &dds, sizeof(dds));

        void* data = GetTextureData(texture);
        if (data)
        {
            qWrite(file, data, texture->mImageDataByteSize);
            qFree(data);
        }

        qClose(file);
    }

    qString FindTextureFile(const char* folder, u32 name_uid)
    {
        qString filename = folder;
        if (!filename.EndsWith("/") && !filename.EndsWith("\\")) {
            filename += "\\";
        }

        auto texture = static_cast<Illusion::Texture*>(qResourceWarehouse::Instance()->DebugGet(RTypeUID_Texture, name_uid));
        if (!texture)
        {
            filename.Format("%08X", name_uid);
            return filename;
        }

        filename += texture->mDebugName;
        filename += ".dds";

        if (!qFileExists(filename)) {
            ExportTexture(texture, filename);
        }

        return filename;
    }
};
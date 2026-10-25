#include "Setup.h"
#include <filesystem>
#include <fstream>
#include <iostream>
int wmain(int count,wchar_t** args){
    namespace fs=std::filesystem;
    wchar_t temp[MAX_PATH]{};if(!GetTempPathW(MAX_PATH,temp))return 1;GUID id{};if(FAILED(CoCreateGuid(&id)))return 1;wchar_t text[40]{};StringFromGUID2(id,text,40);
    fs::path file=fs::path(temp)/(std::wstring(L"CameraPolicy-")+text+L".inf");
    struct Remove{fs::path path;~Remove(){DeleteFileW(path.c_str());}} cleanup{file};
    unsigned checks=0;auto require=[&](bool value){++checks;if(!value){std::cerr<<"Package policy check failed: "<<checks<<'\n';std::exit(1);}};
    auto write=[&](const std::string& hardware,const std::string& provider,const std::string& catalog){
        std::ofstream out(file,std::ios::binary|std::ios::trunc);
        out<<"[Version]\r\nSignature=\"$Windows NT$\"\r\nClass=Camera\r\nClassGUID={CA3E7AB9-B4C3-4AE6-8251-579EF933890F}\r\nProvider=%Provider%\r\nCatalogFile="<<catalog<<"\r\n[Manufacturer]\r\n%Provider%=Models,NTamd64.10.0...19041\r\n[Models.NTamd64.10.0...19041]\r\nDevice=CameraInstall,"<<hardware<<"\r\n[CameraInstall]\r\n[Strings]\r\nProvider=\""<<provider<<"\"\r\n";out.close();require(!out.fail());
    };
    write("ROOT\\TitanVirtualCamera","Titanfyre","TitanVirtualCamera.cat");require(Setup::PackageMatches(file.wstring()));require(!Setup::CatalogTrusted(file.wstring()));
    write("ROOT\\DifferentCamera","Titanfyre","TitanVirtualCamera.cat");require(!Setup::PackageMatches(file.wstring()));
    write("ROOT\\TitanVirtualCamera","AnotherPublisher","TitanVirtualCamera.cat");require(!Setup::PackageMatches(file.wstring()));
    write("ROOT\\TitanVirtualCamera","Titanfyre","AnotherCamera.cat");require(!Setup::PackageMatches(file.wstring()));
    require(!Setup::PackageMatches((file.wstring()+L".absent")));require(!Setup::CatalogTrusted(file.wstring()+L".absent"));
    if(count==2){
        fs::path package=args[1];require(Setup::SignedPackageTrusted(package.wstring()));
        fs::path copy=fs::path(temp)/(std::wstring(L"CameraSignedPolicy-")+text);
        require(CreateDirectoryW(copy.c_str(),nullptr)!=FALSE);
        struct Directory{fs::path p;~Directory(){for(auto name:{L"TitanVirtualCamera.inf",L"TitanVirtualCamera.cat",L"TitanVirtualCamera.sys"})DeleteFileW((p/name).c_str());RemoveDirectoryW(p.c_str());}} remove{copy};
        for(auto name:{L"TitanVirtualCamera.inf",L"TitanVirtualCamera.cat",L"TitanVirtualCamera.sys"})require(CopyFileW((package/name).c_str(),(copy/name).c_str(),TRUE)!=FALSE);
        require(Setup::SignedPackageTrusted(copy.wstring()));
        {std::fstream dll(copy/L"TitanVirtualCamera.sys",std::ios::binary|std::ios::in|std::ios::out);char first=0;dll.read(&first,1);first^=1;dll.seekp(0);dll.write(&first,1);dll.close();require(!dll.fail());}
        require(!Setup::SignedPackageTrusted(copy.wstring()));
        require(CopyFileW((package/L"TitanVirtualCamera.sys").c_str(),(copy/L"TitanVirtualCamera.sys").c_str(),FALSE)!=FALSE);
        {std::ofstream inf(copy/L"TitanVirtualCamera.inf",std::ios::binary|std::ios::app);inf<<"\r\n; altered package\r\n";inf.close();require(!inf.fail());}
        require(!Setup::SignedPackageTrusted(copy.wstring()));
        require(CopyFileW((package/L"TitanVirtualCamera.inf").c_str(),(copy/L"TitanVirtualCamera.inf").c_str(),FALSE)!=FALSE);
        {std::fstream catalog(copy/L"TitanVirtualCamera.cat",std::ios::binary|std::ios::in|std::ios::out);char first=0;catalog.read(&first,1);first^=1;catalog.seekp(0);catalog.write(&first,1);catalog.close();require(!catalog.fail());}
        require(!Setup::SignedPackageTrusted(copy.wstring()));
    }
    std::cout<<checks<<" read-only package identity/trust checks passed; no device installation.\n";return 0;
}

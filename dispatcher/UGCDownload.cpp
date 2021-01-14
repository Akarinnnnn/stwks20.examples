#define _CRT_SECURE_NO_WARNINGS 1

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <thread>

#include <steam_api.h>
#include <stwks20/events.hpp>

int main()
{
	using namespace std;
	bool item_ready = false;
	PublishedFileId_t id = 0;
	{
		AppId_t appid = 322330;
		cout << "Target workshop AppID:";
		cin >> appid;
		std::ofstream(L"./steam_appid.txt", std::ios::trunc) << appid;
	}
	if (!SteamAPI_Init())
		return 80;

	steam::events::LambdaHandler<DownloadItemResult_t> handler(0xFFFFFFFFFFFFFFFFULL,
		[&item_ready](const DownloadItemResult_t* params, bool iofail) -> void
		{
			if (params->m_eResult != k_EResultOK)
			{
				item_ready = true;
				return;
			}

			uint64_t filesize = 0;
			uint32_t timestamp = 0;
			char8_t path[360]{};

			SteamUGC()->GetItemInstallInfo(params->m_nPublishedFileId,
				&filesize,
				(char*)path, 360,
				&timestamp);

			try
			{
				std::filesystem::path dest{ L".\\" + std::to_wstring(params->m_nPublishedFileId) + L".zip" };
				std::filesystem::copy_file(path, dest,
					std::filesystem::copy_options::overwrite_existing);

				std::cout << "UGC ID " << params->m_nPublishedFileId << " At: " << std::filesystem::canonical(dest) << "\n\n\n";
			}
			catch (const std::exception& e)
			{
				cerr << e.what();
			}

			item_ready = true;
		});


	steam::events::mthread_dispatcher::Initialize();
	auto& dispatcher = steam::events::mthread_dispatcher::Get();
	dispatcher.RegisterCallback(handler);

	steam::events::mthread_dispatcher::StartThread();
	uint64_t downloaded = 0, total = 0;

	while (true)
	{
		std::cout << "UGC ID: ";
		std::cin >> id;

		item_ready = false;

		if (!SteamUGC()->DownloadItem(id, true))
		{
			std::cout << "下载失败\n";
			continue;
		}
		do
		{
			if (SteamUGC()->GetItemDownloadInfo(id, &downloaded, &total) && total != 0)
				cout << ((double)downloaded / (double)total) << "%, " << downloaded << '/' << total << " bytes." << endl;
			std::this_thread::sleep_for(100ms);
		} while (!item_ready);
	}
	return 0;
}
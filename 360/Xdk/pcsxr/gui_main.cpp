#include "gui.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include "psxcommon.h"
#include "cdriso.h"
#include "cdrom.h"
#include "r3000a.h"
#include "gpu.h"
#include "../../../libpcsxcore/misc.h"
#include "../../../libpcsxcore/cdrom.h"
#include "sys\Mount.h"
#include "simpleini\SimpleIni.h"
#include "aurora.h"  // swizzy rom loading aurora dash
#include "../../../libpcsxcore/profiler.h"
#include <string>
#include <xuiapp.h>

// Incluir header do plugin GPU para acesso às variáveis de frame limiter
extern "C" {
#include "../../../plugins/xbox_soft/fps.h"
}

std::string gameprofile;

// Debug log simples para gameprofile
static FILE* g_debug_log = NULL;

void DebugLog_Init() {
    if (!g_debug_log) {
        g_debug_log = fopen("game:\\debug_log.txt", "a");
        if (g_debug_log) {
            fprintf(g_debug_log, "\n=== Debug Log Iniciado ===\n");
            fflush(g_debug_log);
        }
    }
}

void DebugLog(const char* fmt, ...) {
    if (!g_debug_log) DebugLog_Init();
    if (g_debug_log) {
        va_list args;
        va_start(args, fmt);
        vfprintf(g_debug_log, fmt, args);
        va_end(args);
        fprintf(g_debug_log, "\n");
        fflush(g_debug_log);
    }
}

void RenderXui();
HXUIOBJ hMainScene;

DWORD * InGameThread() {
	while(1) {
		XINPUT_STATE InputState;
		DWORD XInputErr=XInputGetState(0, &InputState);
		if (XInputErr != ERROR_SUCCESS)
			continue;
		
		// Menu/Exit combo: LB + RB + A + B + X + Y
		if (InputState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER && 
			InputState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER &&
			InputState.Gamepad.wButtons & XINPUT_GAMEPAD_A && 
			InputState.Gamepad.wButtons & XINPUT_GAMEPAD_B &&
			InputState.Gamepad.wButtons & XINPUT_GAMEPAD_X && 
			InputState.Gamepad.wButtons & XINPUT_GAMEPAD_Y) {
			OutputDebugStringA("Menu combo pressed");
			PausePcsx();
			xboxConfig.ShutdownRequested = true;
		}

		// Profiler toggle combo: LB + RB + BACK (Select)
		if (InputState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER && 
			InputState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER &&
			InputState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) {
			OutputDebugStringA("Profiler toggle pressed");
			Profiler_Toggle();
			Sleep(200); // Debounce
		}

		Sleep(50);
	}
	return NULL;
}

#define MAX_FILENAME 256

HRESULT ShowKeyboard(std::wstring & resultText, std::wstring titleText, std::wstring descriptionText, std::wstring defaultText) {
  wchar_t result[MAX_FILENAME];
	
	XOVERLAPPED Overlapped;
	ZeroMemory( &Overlapped, sizeof( XOVERLAPPED ) );

	DWORD dwResult = XShowKeyboardUI(
	XUSER_INDEX_ANY,
	0,
	defaultText.c_str(),
	titleText.c_str(),
	descriptionText.c_str(),
	result,
	MAX_FILENAME,
	&Overlapped
	);

	while(!XHasOverlappedIoCompleted(&Overlapped)) {
		RenderXui();
	}

	resultText = result;

	return (dwResult == ERROR_SUCCESS)? S_OK : E_FAIL;
}

void SaveStatePcsx(int n) {
#if 1
	std::string game = xboxConfig.game;
	std::wstring wgame;
	std::wstring result;

	// Get basename of current game
	game.erase(0, game.rfind('\\')+1);

	get_wstring(game, wgame);

	//if (SUCCEEDED(ShowKeyboard(result, L"", L"", wgame.c_str()))) {
	ShowKeyboard(result, L"Enter the filename of the save states", L"If a file with the same name exists it will overwriten", wgame.c_str());
	{
		std::string save_path;
		std::string save_filename;
		save_path = xboxConfig.saveStateDir + "\\" + game;

		// Create save dir
		CreateDirectoryA(save_path.c_str(), NULL);

		// Save state
		get_string(result, save_filename);

		save_path += "\\" + save_filename;

		SaveState(save_path.c_str());
	}
#else
	std::string game = xboxConfig.game + ".sgz";
	SaveState(game.c_str());
#endif
}

void LoadStatePcsx(std::string save) {
	LoadState(save.c_str());

	// Resume emulation
	ResumePcsx();
}

void LoadStatePcsx(int n) {
	std::string game = xboxConfig.game + ".sgz";
	LoadState(game.c_str());

	// Resume emulation
	ResumePcsx();
}

void ChangeDisc(std::string game){

	CdromId[0] = '\0';
	CdromLabel[0] = '\0';

	SetIsoFile(game.c_str());

	if (ReloadCdromPlugin() < 0) {
      printf("failed to load cdr plugin");
	return;
	      }
    if (CDR_open() < 0) {
      printf("failed to open cdr plugin");
    return;
      }

    SetCdOpenCaseTime(time(NULL) + 2);
	LidInterrupt();

	// Resume emulation
	ResumePcsx();
}

void RenderXui() {

	g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
	0xff000000, 1.0f, 0L );

	if (xboxConfig.Running == true) 
		DrawPcsxSurface();
    
    app.RunFrame();
	app.Render();
	XuiTimersRun();

	// Present the backbuffer contents to the display.
	VideoPresent();
}

CSimpleIniA ini,debuginfo;

bool slowboot;

// Função auxiliar para obter ID do jogo (sempre usa nome do arquivo ISO para consistência)
static void GetGameIdentifier(char* out_id, int max_len) {
	// SEMPRE usar o nome do arquivo ISO (basename de GamePath)
	// Isso garante consistência entre salvar (menu) e carregar (execução)
	out_id[0] = '\0';
	if (xboxConfig.GamePath.empty()) {
		strcpy(out_id, "UNKNOWN");
		return;
	}
	
	// Converter wstring para string e extrair basename
	std::string path;
	get_string(xboxConfig.GamePath, path);
	
	// Encontrar última barra
	size_t last_slash = path.find_last_of("\\/");
	if (last_slash != std::string::npos) {
		path = path.substr(last_slash + 1);
	}
	
	// Remover extensão
	size_t dot = path.find_last_of(".");
	if (dot != std::string::npos) {
		path = path.substr(0, dot);
	}
	
	// Limpar caracteres inválidos para filename (substituir por _)
	for (size_t i = 0; i < path.length(); i++) {
		if (path[i] == ' ' || path[i] == '[' || path[i] == ']' || 
		    path[i] == '(' || path[i] == ')' || path[i] == '&' ||
		    path[i] == '@' || path[i] == '!' || path[i] == '#' ||
		    path[i] == '%' || path[i] == '^' || path[i] == '*' ||
		    path[i] == '?' || path[i] == '<' || path[i] == '>' ||
		    path[i] == '|' || path[i] == '"' || path[i] == '\'') {
			path[i] = '_';
		}
	}
	
	strncpy(out_id, path.c_str(), max_len-1);
	out_id[max_len-1] = '\0';
	
	if (out_id[0] == '\0') {
		strcpy(out_id, "UNKNOWN");
	}
}

void SaveGameProfile(void) {
	char profile_path[256];
	char game_id[128];
	
	GetGameIdentifier(game_id, sizeof(game_id));
	
	// Criar diretório se não existir
	CreateDirectoryA("game:\\gameprofile", NULL);
	
	// Usar ID do jogo como nome do arquivo
	sprintf(profile_path, "game:\\gameprofile\\%s.txt", game_id);
	
	DebugLog("[SaveGameProfile] Salvando perfil: %s (ID=%s)", profile_path, game_id);
	
	FILE* fp = fopen(profile_path, "w");
	if (!fp) {
		DebugLog("[SaveGameProfile] ERRO: Não foi possível criar arquivo %s", profile_path);
		return;
	}
	
	// Salvar todas as configurações no formato key=value
	fprintf(fp, "# PCSXr360 Game Profile\n");
	fprintf(fp, "# Game ID: %s\n", game_id);
	fprintf(fp, "UseInterpreter=%d\n", xboxConfig.UseInterpreter);
	fprintf(fp, "UseThreadedGpu=%d\n", xboxConfig.UseThreadedGpu);
	fprintf(fp, "DisableSpuIrq=%d\n", xboxConfig.DisableSpuIrq);
	fprintf(fp, "DisableFrameLimiter=%d\n", xboxConfig.DisableFrameLimiter);
	fprintf(fp, "DisableFrameSkip=%d\n", xboxConfig.DisableFrameSkip);
	fprintf(fp, "UseParasiteEveFix=%d\n", xboxConfig.UseParasiteEveFix);
	fprintf(fp, "UseDarkForcesFix=%d\n", xboxConfig.UseDarkForcesFix);
	fprintf(fp, "UseSlowBoot=%d\n", xboxConfig.UseSlowBoot);
	fprintf(fp, "UseLinearFilter=%d\n", xboxConfig.UseLinearFilter);
	fprintf(fp, "UseCpuBias=%d\n", xboxConfig.UseCpuBias);
	fprintf(fp, "UseTombRaider2Fix=%d\n", xboxConfig.UseTombRaider2Fix);
	fprintf(fp, "UseFrontMission3Fix=%d\n", xboxConfig.UseFrontMission3Fix);
	fprintf(fp, "Shader=%s\n", xboxConfig.Shaders);
	
	fclose(fp);
	DebugLog("[SaveGameProfile] Perfil salvo com sucesso");
}

void LoadGameProfile(void) {
	char profile_path[256];
	char game_id[128];
	
	GetGameIdentifier(game_id, sizeof(game_id));
	
	// Usar ID do jogo como nome do arquivo
	sprintf(profile_path, "game:\\gameprofile\\%s.txt", game_id);
	
	DebugLog("[LoadGameProfile] Carregando perfil: %s (ID=%s)", profile_path, game_id);
	
	FILE* fp = fopen(profile_path, "r");
	if (!fp) {
		DebugLog("[LoadGameProfile] AVISO: Perfil não encontrado para %s, usando defaults", game_id);
		return;
	}
	
	char line[512];
	while (fgets(line, sizeof(line), fp)) {
		// Remover newline
		line[strcspn(line, "\n")] = 0;
		
		// Ignorar comentários e linhas vazias
		if (line[0] == '#' || line[0] == '\0') continue;
		
		// Parse key=value
		char* key = line;
		char* value = strchr(line, '=');
		if (!value) continue;
		*value = '\0';
		value++;
		
		// Carregar valores
		if (strcmp(key, "UseInterpreter") == 0) xboxConfig.UseInterpreter = atoi(value);
		// Compatibilidade com arquivos antigos (UseDynarec)
		else if (strcmp(key, "UseDynarec") == 0) xboxConfig.UseInterpreter = !atoi(value);  // Invertido: UseDynarec=1 -> UseInterpreter=0
		else if (strcmp(key, "UseThreadedGpu") == 0) xboxConfig.UseThreadedGpu = atoi(value);
		else if (strcmp(key, "DisableSpuIrq") == 0) xboxConfig.DisableSpuIrq = atoi(value);
		else if (strcmp(key, "DisableFrameLimiter") == 0) xboxConfig.DisableFrameLimiter = atoi(value);
		else if (strcmp(key, "DisableFrameSkip") == 0) xboxConfig.DisableFrameSkip = atoi(value);
		else if (strcmp(key, "UseParasiteEveFix") == 0) xboxConfig.UseParasiteEveFix = atoi(value);
		else if (strcmp(key, "UseDarkForcesFix") == 0) xboxConfig.UseDarkForcesFix = atoi(value);
		else if (strcmp(key, "UseSlowBoot") == 0) xboxConfig.UseSlowBoot = atoi(value);
		else if (strcmp(key, "UseLinearFilter") == 0) xboxConfig.UseLinearFilter = atoi(value);
		else if (strcmp(key, "UseCpuBias") == 0) xboxConfig.UseCpuBias = atoi(value);
		else if (strcmp(key, "UseTombRaider2Fix") == 0) xboxConfig.UseTombRaider2Fix = atoi(value);
		else if (strcmp(key, "UseFrontMission3Fix") == 0) xboxConfig.UseFrontMission3Fix = atoi(value);
		else if (strcmp(key, "Shader") == 0) strcpy(xboxConfig.Shaders, value);
	}
	
	fclose(fp);
	DebugLog("[LoadGameProfile] Perfil carregado com sucesso");
}

void ApplySettings(const char* path) {
	Config.Cpu        = xboxConfig.UseInterpreter ? CPU_INTERPRETER : CPU_DYNAREC;  // 0 = Dynarec (padrão), 1 = Interpreter
	Config.RCntFix    = xboxConfig.UseParasiteEveFix;
	spuirq            = !xboxConfig.DisableSpuIrq;  // Invert: Disable=0 -> spuirq=1 (ON), Disable=1 -> spuirq=0 (OFF)
	
	// Frame Limiter: Invertido - unchecked = ativo (padrão), checked = desativado
	DebugLog("[ApplySettings] DisableFrameLimiter=%d, DisableFrameSkip=%d", xboxConfig.DisableFrameLimiter, xboxConfig.DisableFrameSkip);
	if (xboxConfig.DisableFrameLimiter) {
		UseFrameLimit = 0;
		iFrameLimit = 0;
		DebugLog("[ApplySettings] FrameLimiter DESATIVADO (UseFrameLimit=0)");
	} else {
		UseFrameLimit = 1;
		iFrameLimit = 2;
		SetAutoFrameCap();
		DebugLog("[ApplySettings] FrameLimiter ATIVADO (UseFrameLimit=1)");
	}
	
	// Frame Skip: Invertido - unchecked = adaptativo (padrão), checked = desativado
	if (xboxConfig.DisableFrameSkip) {
		s_adaptive_skip_enabled = 0;
		UseFrameSkip = 0;
		DebugLog("[ApplySettings] FrameSkip DESATIVADO");
	} else {
		UseFrameSkip = 0;
		DebugLog("[ApplySettings] FrameSkip ATIVO/Adaptativo");
	}
	
	linearfilter      = xboxConfig.UseLinearFilter;
	slowboot          = xboxConfig.UseSlowBoot;
	tombraider2fix    = xboxConfig.UseTombRaider2Fix;
	frontmission3fix  = xboxConfig.UseFrontMission3Fix;
	darkforcesfix     = xboxConfig.UseDarkForcesFix;

	if(xboxConfig.UseCpuBias) {
		Config.CpuBias = 3;
	} else {
		Config.CpuBias = 2;
	}

	// Salvar perfil do jogo usando CdromId
	SaveGameProfile();
}

void LoadSettings() {
	DebugLog("[LoadSettings] Iniciando carregamento de perfil...");
	
	// Inicializar com valores padrão primeiro
	xboxConfig.UseInterpreter = 0;       // 0 = Dynarec (padrão), 1 = Interpreter
	xboxConfig.UseThreadedGpu = 0;       // Threaded GPU desativado
	xboxConfig.DisableSpuIrq = 0;        // 0 = SPU IRQ ON (padrão/mais compatível), 1 = SPU IRQ OFF
	xboxConfig.DisableFrameLimiter = 0;  // Frame limiter ATIVO (0 = não desativa)
	xboxConfig.DisableFrameSkip = 0;     // Frame skip ATIVO (0 = não desativa)
	xboxConfig.UseParasiteEveFix = 0;    // Fix desativado
	xboxConfig.UseDarkForcesFix = 0;     // Fix desativado
	xboxConfig.UseSlowBoot = 0;          // Slow boot desativado
	xboxConfig.UseLinearFilter = 0;      // Linear filter desativado
	xboxConfig.UseCpuBias = 0;           // CPU bias padrão
	xboxConfig.UseTombRaider2Fix = 0;    // Fix desativado
	xboxConfig.UseFrontMission3Fix = 0;  // Fix desativado
	strcpy(xboxConfig.Shaders, "game:\\hlsl\\stock.cg");  // Shader padrão
	
	DebugLog("[LoadSettings] Defaults definidos: FrameLimiter=%d, FrameSkip=%d", 
		xboxConfig.DisableFrameLimiter, xboxConfig.DisableFrameSkip);
	
	// Carregar perfil do jogo (vai sobrescrever os defaults se existir)
	LoadGameProfile();
	
	DebugLog("[LoadSettings] Finalizado: FrameLimiter=%d, FrameSkip=%d", 
		xboxConfig.DisableFrameLimiter, xboxConfig.DisableFrameSkip);
}


void ApplyShader(std::string game){
	ini.SetValue("pcsx", "Shader",xboxConfig.Shaders);

    game = xboxConfig.game;
    // Get basename of current game
	game.erase(0, game.rfind('\\')+1);
    //game = "game:\\gameshader\\" + game;
	game = "game:\\gameprofile\\" + game;
    game = game + ".ini";

	ini.SaveFile(game.c_str());
}

void LoadShader(std::string game){
	ini.Reset();
	game = xboxConfig.game;
    // Get basename of current game
	game.erase(0, game.rfind('\\')+1);
    game = "game:\\gameprofile\\" + game;
    game = game + ".ini";
    //Merge cfg file
	ini.LoadFile(game.c_str());

    strcpy(xboxConfig.Shaders, strdup(ini.GetValue("pcsx", "Shader", "game:\\hlsl\\stock.cg")));
}
// Autoboot ISO wip 1.0
void LoadAutobootIsoConfig()
{
	//todo add costum iso2load
	ini.Reset();
	std::string cfg;
	cfg = "game:\\pcsx.ini";
	ini.LoadFile(cfg.c_str());
	xboxConfig.AutobootIso = ini.GetLongValue("pcsx", "AutobootIso",    0);
	xboxConfig.saveStateDir =    strdup(ini.GetValue("pcsx", "saveStateDir", "game:\\states"));
    strcpy(Config.Bios,          strdup(ini.GetValue("pcsx", "Bios", "SCPH1001.BIN")));
	strcpy(Config.BiosDir,       strdup(ini.GetValue("pcsx", "BiosDir", "game:\\BIOS")));
	strcpy(Config.Mcd1,          strdup(ini.GetValue("pcsx", "Mcd1", "game:\\memcards\\Memcard1.mcd")));
	strcpy(Config.Mcd2,          strdup(ini.GetValue("pcsx", "Mcd2", "game:\\memcards\\Memcard2.mcd")));


	//save settimngs
	
	ini.SetLongValue("pcsx", "AutobootIso", xboxConfig.AutobootIso);
	ini.SetValue("pcsx", "saveStateDir", xboxConfig.saveStateDir.c_str());
	ini.SetValue("pcsx", "Bios", Config.Bios);	
	ini.SetValue("pcsx", "BiosDir", Config.BiosDir);
	ini.SetValue("pcsx", "Mcd1", Config.Mcd1);
	ini.SetValue("pcsx", "Mcd2", Config.Mcd2);

	ini.SaveFile(cfg.c_str());
}

void InitPcsx() {
	// Initialize pcsx with default settings
	XMemSet(&Config, 0, sizeof(PcsxConfig));

    // Create dir ...
	CreateDirectory("game:\\states\\",               NULL);
	CreateDirectory("game:\\memcards\\",             NULL);
	CreateDirectory("game:\\gameprofile\\",          NULL);
	CreateDirectory("game:\\covers\\",               NULL);
	CreateDirectory("game:\\gameguides\\",           NULL);
	//CreateDirectory("game:\\gameshader\\",           NULL);
	CreateDirectory("game:\\ROMS\\",           NULL);
	CreateDirectory("game:\\BIOS\\",           NULL);
	CreateDirectory(xboxConfig.saveStateDir.c_str(), NULL);

	Config.PsxOut                  = 1;
	Config.HLE                     = 0;
    Config.Xa                      = 0;  //XA enabled
	Config.Cdda                    = 0;
	Config.PsxAuto                 = 0;  
	Config.CpuBias                 = 2;  //new!!  2 is the standard value. 3 the emulator will run faster
	xboxConfig.noGameBrowse        = 0;
	xboxConfig.CoverModeHorizontal = 1; //setup covermode 
	xboxConfig.AutobootIsoFromAurora = 0;


	// 27/03/17  add info type msg box
	xboxConfig.InfoBoxType = 0;
	xboxConfig.InfoboxShow = false;
	xboxConfig.Infomsg = L"";
	/*
		int InfoBoxType; //1 - Information ;  2 - Error
	bool InfoboxShow;
	std::wstring Infomsg;*/

}

//void DoPcsx(char* game) {
void DoPcsx(std::string sgame){
  int ret;
  //std::string sgame;
  char *path= 0;

    Config.PsxAuto = TRUE;

 //   sgame = game;
    LoadShader(sgame);
	ApplySettings(path);

    POKOPOM_Init();
	SetIsoFile((char*)sgame.c_str());
	gpuDmaThreadInit();

	if (SysInit() == -1) {
		// Display an error ?
		printf("SysInit() Error!\n");
		return;
	}

	gpuThreadEnable(xboxConfig.UseThreadedGpu);

	ret = CDR_open();
	if (ret < 0) { SysMessage (_("Error Opening CDR Plugin")); return; }
	
	// Recarregar perfil após CDR_open para usar CdromId se disponível
	LoadSettings();
	// Reaplicar configurações carregadas aos variáveis de runtime
	ApplySettings(NULL);
	
		ret = GPU_open(NULL);
	if (ret < 0) { SysMessage (_("Error Opening GPU Plugin (%d)"), ret); return; }
		ret = SPU_open(NULL);
	
	SPU_registerCallback(SPUirq);
	
	if (ret < 0) { SysMessage (_("Error Opening SPU Plugin (%d)"), ret); return; }
		ret = PAD1_open(NULL);
	if (ret < 0) { SysMessage (_("Error Opening PAD1 Plugin (%d)"), ret); return; }
		PAD1_registerVibration(GPU_visualVibration);
		ret = PAD2_open(NULL);
	if (ret < 0) { SysMessage (_("Error Opening PAD2 Plugin (%d)"), ret); return; }	
		PAD2_registerVibration(GPU_visualVibration); 

	GPU_clearDynarec(clearDynarec);
	Config.SlowBoot = (int)slowboot;


	SysPrintf("CheckCdrom\r\n");
	int res = CheckCdrom();
	if(res)
		SysPrintf("CheckCdrom: %08x\r\n",res);
    SysReset();
    res=LoadCdrom();
	if(res)
		SysPrintf("LoadCdrom: %08x\r\n",res);

	LoadShaderFromFile(xboxConfig.Shaders);
	CoverSystemAndGameProfile(getgameID());

	// Initialize profiler with game ID after CDROM is loaded
	if (CdromId[0] != '\0') {
		Profiler_Init(CdromId, CdromLabel);
		SysPrintf("Profiler initialized for game: %s (%s)\r\n", CdromId, CdromLabel);
	}

	SysPrintf("Execute\r\n");
	psxCpu->Execute();
}

void CoverSystemAndGameProfile(std::string gameID) {	
	
	get_string(xboxConfig.GamePath, gameprofile);
    //Get basename of current game
	gameprofile.erase(0, gameprofile.rfind('\\')+1);
    gameprofile = "game:\\gameprofile\\" + gameprofile;
    gameprofile = gameprofile + ".ini";
    wsprintf(xboxConfig.CurrentgameID,gameprofile.c_str());
}


static void InitXui() {
	// Initialize the xui application
	HRESULT hr = app.InitShared(g_pd3dDevice, &g_d3dpp, XuiD3DXTextureLoader, NULL);

    // Register a default typeface
    hr = app.RegisterDefaultTypeface( L"Arial Unicode MS", L"file://game:/media/PsxSkin.xzp#media/Graphics/xarialuni.ttf" );

    // Load the skin file used for the scene. 
	app.LoadSkin( L"file://game:/media/PsxSkin.xzp#media/Graphics/simple_scene_skin.xur" );
	
	XuiSceneCreate(L"file://game:/media/PsxSkin.xzp#media/Graphics/", L"scene.xur", NULL, &hMainScene);

    // Load the scene.
	app.LoadFirstScene( L"file://game:/media/PsxSkin.xzp#media/Graphics/", L"scene.xur", NULL );

	// Start the in game thread
	HANDLE hInGameThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)InGameThread, NULL, CREATE_SUSPENDED, NULL);

	XSetThreadProcessor(hInGameThread, 5);
	ResumeThread(hInGameThread);

    // Set matrice for xui - allow to resize move correctly in small tvs
	D3DXMATRIX mat;
	D3DXMATRIX tr;

	XuiRenderGetViewTransform(app.GetDC(), &mat);
	D3DXMatrixTranslation(&tr, 0, (((float)g_d3dpp.BackBufferHeight-720.f)/2.f), 0);
	D3DXMatrixMultiply(&mat, &mat, &tr);
	XuiRenderSetViewTransform(app.GetDC(), &mat);
}

void Add(char* d, char* mountpoint){
	Map(mountpoint,d);
}

HRESULT MountDevices(){

	Add("\\Device\\Cdrom0","cdrom:");

	//Add("\\Device\\Flash","flash:");

	//hdd
	Add("\\Device\\Harddisk0\\Partition0","hdd0:");
	Add("\\Device\\Harddisk0\\Partition1","hdd1:");
	Add("\\Device\\Harddisk0\\Partition2","hdd2:");
	Add("\\Device\\Harddisk0\\Partition3","hdd3:");

	//mu
	Add("\\Device\\Mu0","mu0:");
	Add("\\Device\\Mu1","mu1:");
	Add("\\Device\\Mass0PartitionFile\\Storage","usbmu0:");

	//usb
	Add("\\Device\\Mass0", "usb0:");
	Add("\\Device\\Mass1", "usb1:");
	Add("\\Device\\Mass2", "usb2:");

	Add("\\Device\\BuiltInMuSfc","sfc:");

	return S_OK;
}

VOID __cdecl main(){
	
	MountDevices();
	InitD3D();
	InitPcsx();
	InitXui();
	//autoboot Iso setup
	LoadAutobootIsoConfig();
	//Check for bios In folder if is not available show msg and quit emulator
	std::string s = "game:\\bios\\" + (std::string)Config.Bios;
	if(!fileExists(s.c_str())) // no bios no boot!!!
	{
			//std::wstring wpath = xboxConfig.GamePath ;
		   // ShowMessageBoxEx(NULL,NULL,L"PCSXR360 System Error",L"Bios file not found. Please check your settings", 1, (LPCWSTR*)L"OK",NULL,  XUI_MB_CENTER_ON_PARENT, NULL);
		xboxConfig.InfoBoxType = 2; //1 - Information ;  2 - Error
		xboxConfig.InfoboxShow = true;
		xboxConfig.Infomsg = L"Bios file not found. Please check your settings";
	}

	if (xboxConfig.AutobootIso){
		xboxConfig.game = "game:\\psx.iso";
		xboxConfig.GamePath = std::wstring(xboxConfig.game.begin(),xboxConfig.game.end()) ;
		LoadSettings();
		xboxConfig.noGameBrowse = true;
		xboxConfig.AutobootIsoFromAurora = false;

	}

	while(1) {
		// Render

		RenderXui();

		//new code to execute roms from Aurora Dash wip
		//------------------------------------------------------------------------
		
		DWORD dwLaunchDataSize;
		if (XGetLaunchDataSize(&dwLaunchDataSize) == ERROR_SUCCESS) {
			BYTE* pLaunchData = new BYTE[dwLaunchDataSize];
			XGetLaunchData(pLaunchData, dwLaunchDataSize);
			AURORA_LAUNCHDATA_ROM_V2* aurora = (AURORA_LAUNCHDATA_ROM_V2*)pLaunchData;
			char* extracted_path = new char[dwLaunchDataSize];
			memset(extracted_path, 0, dwLaunchDataSize);
			if (aurora->ApplicationId == AURORA_LAUNCHDATA_APPID && aurora->FunctionId == AURORA_LAUNCHDATA_ROM_FUNCID && (aurora->FunctionVersion == 1 || aurora->FunctionVersion == 2)) {
				if (xbox_io_mount("aurora:", aurora->SystemPath) >= 0)
					sprintf_s(extracted_path, dwLaunchDataSize, "aurora:%s%s", aurora->RelativePath, aurora->Exectutable);
				//TODO: mention error
			}
			if (pLaunchData)
				delete []pLaunchData; // Cleanup memory
			if (extracted_path && extracted_path[0] != 0) {
				xboxConfig.Running = true;
				xboxConfig.game = extracted_path;
				xboxConfig.GamePath = std::wstring(xboxConfig.game.begin(),xboxConfig.game.end()) ;
				xboxConfig.noGameBrowse = true;
				xboxConfig.AutobootIsoFromAurora = true;
				LoadSettings();
				//wsprintf(xboxConfig.Game2Boot,xboxConfig.game.c_str());
				RemoveSound();	
				
				cdrIsoInit();
				//DoPcsx((char*)xboxConfig.game.c_str());
				DoPcsx(xboxConfig.game);
			}
		}else{
				// a game can be loaded !
				if (!xboxConfig.game.empty() && !xboxConfig.Running) {
				// run it !!!			
					xboxConfig.Running = true;
					/*
					if (xboxConfig.AutobootIso){
						xboxConfig.GamePath = std::wstring(xboxConfig.game.begin(),xboxConfig.game.end()) ;
						LoadSettings();
					}*/

					RemoveSound();
					cdrIsoInit();
					//DoPcsx((char*)xboxConfig.game.c_str());

					DoPcsx(xboxConfig.game);
				}
				
			}
	    }
    // Free resources, unregister custom classes, and exit.
    app.Uninit();
}


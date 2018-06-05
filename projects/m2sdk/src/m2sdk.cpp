#include "zpl.h"
#include "m2sdk.h"

#define mod_assert ZPL_ASSERT

// include source code parts
#include "temp.hpp"
#include "memory.hpp"
#include "entity.hpp"
#include "steamdrm.hpp"

M2::gamemodule_cb g_gamemodule_callback = nullptr;

DWORD GameStartDrive__Return;
DWORD GameStartDrive_2__Return;
DWORD GameStartDrive_3__Return;
DWORD GameEndDrive__Return;
DWORD _callDrive = 0x042CAC0;
DWORD _callEnd = 0x99CE70;


bool player_request_vehicle_enter(M2::C_Car *car) {
    m2sdk_log("[game-event] ped request vehicle enter\n");
    return true;
}

void game_player_enter_car(M2::C_Player2 *player, M2::C_Actor *car, u8 seat) {
    m2sdk_log("[game-event] ped entering the car on seat: %d\n", seat);
}
/**
* Hooking vehicle methods 
*/
void __declspec(naked) GameStartDriveHook__1()
{
    __asm call[_callDrive];
    __asm pushad;
    //player_mod_message(E_PlayerMessage::MESSAGE_MOD_BREAKIN_CAR);
    __asm popad;
    __asm jmp[GameStartDrive__Return];
}

void __declspec(naked) GameStartDriveHook__2()
{
    __asm call[_callDrive];
    __asm pushad;
    //player_mod_message(E_PlayerMessage::MESSAGE_MOD_BREAKIN_CAR);
    __asm popad;
    __asm jmp[GameStartDrive_2__Return];
}

static M2::C_Car *car = nullptr;
void __declspec(naked) GameStartDriveHook__3()
{
    __asm call[_callDrive];
    __asm pushad
    //player_mod_message(E_PlayerMessage::MESSAGE_MOD_ENTER_CAR);
    __asm popad;
    __asm jmp[GameStartDrive_3__Return];
}

void __declspec(naked) GameEndDriveHook()
{
    __asm call[_callEnd];
    __asm pushad;
    //player_mod_message(E_PlayerMessage::MESSAGE_MOD_LEAVE_CAR);
    __asm popad;
    __asm jmp[GameEndDrive__Return];
}

DWORD CPlayer__EnterCar__Call = 0x42CAC0;
DWORD CPlayer__EnterCar_JumpBack = 0x437945;
void __declspec(naked) CPlayer2__EnterCar()
{
    __asm
    {
        mov eax, dword ptr ss : [esp + 0x10]
        mov ecx, dword ptr ds : [edi + 0x44]

        pushad
        push eax
        push ecx
        push esi
        call game_player_enter_car
        add esp, 0xC
        popad

        push eax
        push ecx
        mov ecx, esi
        call CPlayer__EnterCar__Call
        jmp CPlayer__EnterCar_JumpBack
    }
}

DWORD CHuman2CarWrapper__GetCar = 0x9235F0;
DWORD CHuman2CarWrapper__GetDoor = 0x940C80;
static M2::C_Car *tryToEnterCar = nullptr;
void __declspec(naked) CHuman2CarWrapper__IsFreeToGetIn__Hook()
{
    __asm
    {
        mov ecx, esi;
        call CHuman2CarWrapper__GetCar;
        mov tryToEnterCar, eax;
    }


    if (player_request_vehicle_enter(tryToEnterCar) == true) {
        __asm {
            mov     al, 1
            pop     esi
            retn    8
        }
    }
    else {
        __asm {
            mov     al, 0
            pop     esi
            retn    8
        }
    }
}

/**
* Hooks for bugfixes
*/
void *_this;
void _declspec(naked) FrameReleaseFix()
{
    __asm
    {
        pushad;
        mov _this, esi;
    }

    //TODO: Check if _this != nullptr

    __asm
    {
        popad;
        retn;
    }
}

void _declspec(naked) FrameReleaseFix2()
{
    //TODO: Check if _this != nullptr
    __asm
    {
        retn;
    }
}

/* Entity Messages */
typedef bool(__cdecl * CScriptEntity__RecvMessage_t) (void *lua, void *a2, const char *function, M2::C_EntityMessage *message);
CScriptEntity__RecvMessage_t onReceiveMessage;
int OnReceiveMessageHook(void *lua, void *a2, const char *function, M2::C_EntityMessage *pMessage)
{
    if (pMessage) {
        M2::C_Game *game = M2::C_Game::Get();
        if (game) {
            M2::C_Player2 *player = game->GetLocalPed();
            if (player) {
                M2::C_Entity *entity = reinterpret_cast<M2::C_Entity*>(player);
                if (entity) {
                    if (pMessage->m_dwReceiveGUID == entity->m_dwGUID) {
                        //player_game_message(pMessage);
                    }
                }
            }
        }
    }
    return onReceiveMessage(lua, a2, function, pMessage);
}

/* Player input process */
DWORD CPlayer2__UpdateInput__Return;
DWORD CPlayer2__UpdateInput__Call = 0x42ABE0;
M2::C_Entity *player = nullptr;
void __declspec(naked) CPlayer2__UpdateInput()
{
    __asm call[CPlayer2__UpdateInput__Call];
    __asm mov player, ebx;
    __asm pushad;

    //TODO: Hook here

    __asm popad;
    __asm jmp[CPlayer2__UpdateInput__Return];
}

/* Human death */
DWORD MineDeathHook_JumpBack = 0x00990CFF;
DWORD _CHuman2__SetupDeath = 0x0098C160;

void OnHumanSetupDeath(M2::C_Human2* human, M2::C_EntityMessageDamage* damage)
{
    if (human == reinterpret_cast<M2::C_Human2*>(M2::C_Game::Get()->GetLocalPed())) {
        m2sdk_log("The player just died\n");
    }
    else {
        m2sdk_log("An human just died\n");
    }
}

__declspec(naked) void CHuman2__SetupDeath_Hook()

{
    __asm
    {
        pushad
        push esi
        push ebp
        call OnHumanSetupDeath
        add esp, 0x8
        popad

        push    esi
        mov     ecx, ebp
        call    _CHuman2__SetupDeath

        jmp MineDeathHook_JumpBack
    }
}

DWORD _CHuman2__DoDamage = 0x09907D0;
DWORD _DoDamage__JumpBack = 0x042FC6F;
void OnHumanDoDamage(M2::C_Human2 *human, M2::C_EntityMessageDamage *message)
{
    //Do things here
}

__declspec(naked) void CHuman2__DoDamage__Hook()
{
    __asm
    {
        pushad;
        push esi;
        push edi;
        call OnHumanDoDamage;
        add esp, 0x8;
        popad;

        push edi;
        mov ecx, esi;
        call _CHuman2__DoDamage;

        jmp _DoDamage__JumpBack;
    }
}

/* Actions patching */
void __declspec(naked) CCarActionEnter__TestAction__Hook()
{
    __asm {
        pop     edi;
        mov     al, 1;
        pop     esi;
        add     esp, 0Ch;
        retn    4;
    }
}

void __declspec(naked) CCarActionBreakIn__TestAction__Hook()
{
    __asm {
        pop     edi
        pop     esi
        mov     al, 0
        pop     ebx
        retn    4
    }
}
/*
DWORD _CHuman2__AddCommand;
void __declspec(naked) CHuman2__AddCommand()
{
    __asm mov     eax, [esp + 4];
    __asm push    esi;



    __asm pushad;

    static int cmdtype;
    __asm mov cmdtype, eax;

    static char* cmd;
    __asm mov edi, [esp + 16];
    __asm mov cmd, edi;

    mod_log("CHuman2__AddCommand: type %d humancmdptr %x\n", cmdtype, cmd);
    __asm popad;


    __asm jmp[_CHuman2__AddCommand];
}*/

DWORD _CHuman2__AddCommand;
void __declspec(naked) CHuman2__AddCommand()
{
    __asm mov     eax, [esp + 4];
    __asm push    esi;



    __asm pushad;

    static M2::E_Command cmdtype;
    __asm mov cmdtype, eax;

    static char* cmd;
    __asm mov edi, [esp + 16];
    __asm mov cmd, edi;

    m2sdk_log("CHuman2__AddCommand: type %d humancmdptr %x", cmdtype, cmd);
    __asm popad;


    __asm jmp[_CHuman2__AddCommand];
}


/* Game Module Implementation */
DWORD __GameModuleInstall = 0x4F2F0A;
void __declspec(naked) GameModuleInstall()
{
    __asm {
        mov eax, [edx + 1Ch];
        push 0Ah;
    }
    __asm pushad;
    g_gamemodule_callback();
    __asm popad;
    __asm jmp[__GameModuleInstall];
}

DWORD __LoadCityPart;
void __declspec(naked) LoadCityPartsHook()
{
    __asm {
        push ebx;
        push ebp;
        push esi;
        push edi;
        mov edi, [ecx + 16];
    }
    __asm pushad;
    m2sdk_log("load city part\n");
    __asm popad;
    __asm jmp[__LoadCityPart];
}


void M2::Initialize(gamemodule_cb callback) {
    g_gamemodule_callback = callback;

    Mem::Initialize();
    steam_drm_install();

    // Hooking game module registering
    Mem::Hooks::InstallJmpPatch(0x4F2F05, (DWORD)GameModuleInstall, 5);

    // vehicle hooks
    GameStartDrive__Return = Mem::Hooks::InstallNotDumbJMP(0x043B305, (M2_Address)GameStartDriveHook__1);
    GameStartDrive_2__Return = Mem::Hooks::InstallNotDumbJMP(0x43B394, (M2_Address)GameStartDriveHook__2);
    GameStartDrive_3__Return = Mem::Hooks::InstallNotDumbJMP(0x437940, (M2_Address)GameStartDriveHook__3);
    GameEndDrive__Return = Mem::Hooks::InstallNotDumbJMP(0x43BAAD, (M2_Address)GameEndDriveHook);
    Mem::Hooks::InstallJmpPatch(0x437935, (M2_Address)CPlayer2__EnterCar);

    // Crash fix on C_Frame::Release
    Mem::Hooks::InstallJmpPatch(0x14E5BC0, (DWORD)FrameReleaseFix);
    Mem::Hooks::InstallJmpPatch(0x12F0DB0, (DWORD)FrameReleaseFix2);

    // Patchs for enter action testing
    Mem::Hooks::InstallJmpPatch(0xA3E8E1, (DWORD)CCarActionEnter__TestAction__Hook);
    Mem::Hooks::InstallJmpPatch(0xA3F0A6, (DWORD)CCarActionBreakIn__TestAction__Hook);
    Mem::Hooks::InstallJmpPatch(0x956143, (DWORD)CHuman2CarWrapper__IsFreeToGetIn__Hook);

    // Hooking human death
    Mem::Hooks::InstallJmpPatch(0x00990CF7, (DWORD)&CHuman2__SetupDeath_Hook);
    Mem::Hooks::InstallJmpPatch(0x042FC63, (DWORD)&CHuman2__DoDamage__Hook);


    //_CHuman2__AddCommand = (DWORD)Mem::Hooks::InstallNotDumbJMP(0x94D400, (DWORD)CHuman2__AddCommand, 5);
    __LoadCityPart = (DWORD)Mem::Hooks::InstallNotDumbJMP(0x4743C0, (DWORD)LoadCityPartsHook, 5);

    // Entity Messages hooks
    onReceiveMessage = (CScriptEntity__RecvMessage_t) Mem::Hooks::InstallJmpPatch(0x117BCA0, (DWORD)OnReceiveMessageHook);

    // Player input hook
    CPlayer2__UpdateInput__Return = Mem::Hooks::InstallNotDumbJMP(0x43BD42, (M2_Address)CPlayer2__UpdateInput);

    // noop the CreateMutex, allow to run multiple instances
    Mem::Hooks::InstallJmpPatch(0x00401B89, 0x00401C16);

    // Always use vec3
    *(BYTE *)0x09513EB = 0x75;
    *(BYTE *)0x0950D61 = 0x75;

    // Disable game controlling engine state and radio
    Mem::Hooks::InstallJmpPatch(0x956362, 0x9563B6); // When leaving car
    Mem::Hooks::InstallJmpPatch(0x95621A, 0x956333); // When entering car

    // Disable game pause when minimized or in background
    Mem::Hooks::InstallJmpPatch(0xAC6D2B, 0xAC6F79);
    Mem::Hooks::InstallJmpPatch(0xAC6E57, 0xAC6F79);

    // Disable game creating vehicle (common/police) map icons
    Mem::Hooks::InstallJmpPatch(0x9CC219, 0x9CC220);//C_Car::OnActivate
    Mem::Hooks::InstallJmpPatch(0x9CC53C, 0x9CC543);//C_Car::OnDeactivate
    Mem::Hooks::InstallJmpPatch(0x4DCABD, 0x4DCAC4);//C_Car::SetSeatOwner
    Mem::Hooks::InstallJmpPatch(0x4DCC7D, 0x4DCC8A);//C_Car::SetSeatOwner

    // Prevent game controlling wipers
    Mem::Hooks::InstallJmpPatch(0x4877F1, 0x487892);//C_Car::UpdateIdleFX
    Mem::Hooks::InstallJmpPatch(0xA151CB, 0xA151D4);//C_Car::InitTuning

    // Disable shop loading
    //Mem::Utilites::PatchAddress(0x4731A0, 0x0004C2);
    //Mem::Utilites::PatchAddress(0xAC4B80, 0x0004C2);

    // Disable garages
    //Mem::Utilites::PatchAddress(0xCD6E90, 0xC300B0);

    // Disable loading screen
    Mem::Utilites::PatchAddress(0x08CA820, 0xC300B0); // mov al, 0; retn

    // Disable DLC loadings (NONO, WE NEED DLCs !)
    //Mem::Utilites::PatchAddress(0x11A62C0, 0xC300B0); // mov al, 0; retn
}

void M2::Free() {}

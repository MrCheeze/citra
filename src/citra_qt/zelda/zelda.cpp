#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <fmt/format.h>

#include <QAbstractTableModel>
#include <QCheckBox>
#include <QDialog>
#include <QEvent>
#include <QFileDialog>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHoverEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QOpenGLWidget>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVBoxLayout>

#include "citra_qt/zelda/game_actor.h"
#include "citra_qt/zelda/game_allocator.h"
#include "citra_qt/zelda/game_common_data.h"
#include "citra_qt/zelda/game_player.h"
#include "citra_qt/zelda/game_types.h"
#include "citra_qt/zelda/search_bar.h"
#include "citra_qt/zelda/zelda.h"
#include "common/common_types.h"
#include "common/file_util.h"
#include "core/core.h"
#include "core/memory.h"

namespace zelda {

namespace {

bool IsActor(const game::AllocatorBlock& block) {
    return !block.IsFree() && block.name.addr == 0x00643A90;
}

Ptr<game::Actor> GetActorFromBlock(Ptr<game::AllocatorBlock> block) {
    if (!IsActor(*block))
        return nullptr;
    return Ptr<game::Actor>{u32(block.addr + sizeof(game::AllocatorBlock))};
}

const char* GetActorName(const game::Actor& actor) {
    static const std::unordered_map<int, const char*> names = {
        {0x0000, "Player"},
        {0x0001, "En_Test"},
        {0x0002, "En_GirlA"},
        {0x0003, "En_Part"},
        {0x0004, "En_Light"},
        {0x0005, "En_Door"},
        {0x0006, "En_Box"},
        {0x0007, "En_Pametfrog"},
        {0x0008, "En_Okuta"},
        {0x0009, "En_Bom"},
        {0x000A, "En_Wallmas"},
        {0x000B, "En_Dodongo"},
        {0x000C, "En_Firefly"},
        {0x000D, "En_Horse"},
        {0x000E, "En_Item00"},
        {0x000F, "En_Arrow"},
        {0x0010, "En_Elf"},
        {0x0011, "En_Niw"},
        {0x0012, "En_Tite"},
        {0x0013, "En_Peehat"},
        {0x0014, "En_Butte"},
        {0x0015, "En_Insect"},
        {0x0016, "En_Fish"},
        {0x0017, "En_Holl"},
        {0x0018, "En_Dinofos"},
        {0x0019, "En_Hata"},
        {0x001A, "En_Zl1"},
        {0x001B, "En_Viewer"},
        {0x001C, "En_Bubble"},
        {0x001D, "Door_Shutter"},
        {0x001E, "En_Boom"},
        {0x001F, "En_Torch2"},
        {0x0020, "En_Minifrog"},
        {0x0021, "En_St"},
        {0x0022, "En_A_Obj"},
        {0x0023, "Obj_Wturn"},
        {0x0024, "En_River_Sound"},
        {0x0025, "En_Ossan"},
        {0x0026, "En_Famos"},
        {0x0027, "En_Bombf"},
        {0x0028, "En_Am"},
        {0x0029, "En_Dekubaba"},
        {0x002A, "En_M_Fire1"},
        {0x002B, "En_M_Thunder"},
        {0x002C, "Bg_Breakwall"},
        {0x002D, "Door_Warp1"},
        {0x002E, "Obj_Syokudai"},
        {0x002F, "Item_B_Heart"},
        {0x0030, "En_Dekunuts"},
        {0x0031, "En_Bbfall"},
        {0x0032, "Arms_Hook"},
        {0x0033, "En_Bb"},
        {0x0034, "Bg_Keikoku_Spr"},
        {0x0035, "En_Wood02"},
        {0x0036, "En_Death"},
        {0x0037, "En_Minideath"},
        {0x0038, "En_Vm"},
        {0x0039, "Demo_Effect"},
        {0x003A, "Demo_Kankyo"},
        {0x003B, "En_Floormas"},
        {0x003C, "En_Rd"},
        {0x003D, "Bg_F40_Flift"},
        {0x003E, "ovl_bg_heavy_block"},
        {0x003F, "Obj_Mure"},
        {0x0040, "En_Sw"},
        {0x0041, "Object_Kankyo"},
        {0x0042, "En_Horse_Link_Child"},
        {0x0043, "Door_Ana"},
        {0x0044, "En_Encount1"},
        {0x0045, "Demo_Tre_Lgt"},
        {0x0046, "En_Encount2"},
        {0x0047, "En_Fire_Rock"},
        {0x0048, "Bg_Ctower_Rot"},
        {0x0049, "Mir_Ray"},
        {0x004A, "En_Sb"},
        {0x004B, "En_Bigslime"},
        {0x004C, "En_Karebaba"},
        {0x004D, "En_In"},
        {0x004E, "En_Ru"},
        {0x004F, "En_Bom_Chu"},
        {0x0050, "En_Horse_Game_Check"},
        {0x0051, "En_Rr"},
        {0x0052, "En_Fr"},
        {0x0053, "En_Fishing"},
        {0x0054, "Obj_Oshihiki"},
        {0x0055, "Eff_Dust"},
        {0x0056, "Bg_Umajump"},
        {0x0057, "Arrow_Fire"},
        {0x0058, "Arrow_Ice"},
        {0x0059, "Arrow_Light"},
        {0x005A, "Item_Etcetera"},
        {0x005B, "Obj_Kibako"},
        {0x005C, "Obj_Tsubo"},
        {0x005D, "En_Ik"},
        {0x005E, "Demo_Shd"},
        {0x005F, "En_Dns"},
        {0x0060, "Elf_Msg"},
        {0x0061, "En_Honotrap"},
        {0x0062, "En_Tubo_Trap"},
        {0x0063, "Obj_Ice_Poly"},
        {0x0064, "En_Fz"},
        {0x0065, "En_Kusa"},
        {0x0066, "Obj_Bean"},
        {0x0067, "Obj_Bombiwa"},
        {0x0068, "Obj_Switch"},
        {0x0069, "Obj_Lift"},
        {0x006A, "Obj_Hsblock"},
        {0x006B, "En_Okarina_Tag"},
        {0x006C, "En_Goroiwa"},
        {0x006D, "En_Daiku"},
        {0x006E, "En_Nwc"},
        {0x006F, "Item_Inbox"},
        {0x0070, "En_Ge1"},
        {0x0071, "Obj_Blockstop"},
        {0x0072, "En_Sda"},
        {0x0073, "En_Clear_Tag"},
        {0x0074, "En_Gm"},
        {0x0075, "En_Ms"},
        {0x0076, "En_Hs"},
        {0x0077, "Bg_Ingate"},
        {0x0078, "En_Kanban"},
        {0x0079, "En_Attack_Niw"},
        {0x007A, "En_Mk"},
        {0x007B, "En_Owl"},
        {0x007C, "En_Ishi"},
        {0x007D, "Obj_Hana"},
        {0x007E, "Obj_Lightswitch"},
        {0x007F, "Obj_Mure2"},
        {0x0080, "En_Fu"},
        {0x0081, "En_Stream"},
        {0x0082, "En_Mm"},
        {0x0083, "En_Weather_Tag"},
        {0x0084, "En_Ani"},
        {0x0085, "En_Js"},
        {0x0086, "En_Okarina_Effect"},
        {0x0087, "En_Mag"},
        {0x0088, "Elf_Msg2"},
        {0x0089, "Bg_F40_Swlift"},
        {0x008A, "En_Kakasi"},
        {0x008B, "Obj_Makeoshihiki"},
        {0x008C, "Oceff_Spot"},
        {0x008D, "En_Torch"},
        {0x008E, "Shot_Sun"},
        {0x008F, "Obj_Roomtimer"},
        {0x0090, "En_Ssh"},
        {0x0091, "Oceff_Wipe"},
        {0x0092, "Oceff_Storm"},
        {0x0093, "Obj_Demo"},
        {0x0094, "En_Minislime"},
        {0x0095, "En_Nutsball"},
        {0x0096, "Oceff_Wipe2"},
        {0x0097, "Oceff_Wipe3"},
        {0x0098, "En_Dg"},
        {0x0099, "En_Si"},
        {0x009A, "Obj_Comb"},
        {0x009B, "Obj_Kibako2"},
        {0x009C, "En_Hs2"},
        {0x009D, "Obj_Mure3"},
        {0x009E, "En_Tg"},
        {0x009F, "En_Wf"},
        {0x00A0, "En_Skb"},
        {0x00A1, "En_Gs"},
        {0x00A2, "Obj_Sound"},
        {0x00A3, "En_Crow"},
        {0x00A4, "En_Cow"},
        {0x00A5, "Oceff_Wipe4"},
        {0x00A6, "En_Zo"},
        {0x00A7, "Obj_Makekinsuta"},
        {0x00A8, "En_Ge3"},
        {0x00A9, "Obj_Hamishi"},
        {0x00AA, "En_Zl4"},
        {0x00AB, "En_Mm2"},
        {0x00AC, "Door_Spiral"},
        {0x00AD, "Obj_Pzlblock"},
        {0x00AE, "Obj_Toge"},
        {0x00AF, "Obj_Armos"},
        {0x00B0, "Obj_Boyo"},
        {0x00B1, "En_Grasshopper"},
        {0x00B2, "Obj_Grass"},
        {0x00B3, "Obj_Grass_Carry"},
        {0x00B4, "Obj_Grass_Unit"},
        {0x00B5, "Bg_Fire_Wall"},
        {0x00B6, "En_Bu"},
        {0x00B7, "En_Encount3"},
        {0x00B8, "En_Jso"},
        {0x00B9, "Obj_Chikuwa"},
        {0x00BA, "En_Knight"},
        {0x00BB, "En_Warp_tag"},
        {0x00BC, "En_Aob_01"},
        {0x00BD, "En_Boj_01"},
        {0x00BE, "En_Boj_02"},
        {0x00BF, "En_Boj_03"},
        {0x00C0, "En_Encount4"},
        {0x00C1, "En_Bom_Bowl_Man"},
        {0x00C2, "En_Syateki_Man"},
        {0x00C3, "Bg_Icicle"},
        {0x00C4, "En_Syateki_Crow"},
        {0x00C5, "En_Boj_04"},
        {0x00C6, "En_Cne_01"},
        {0x00C7, "En_Bba_01"},
        {0x00C8, "En_Bji_01"},
        {0x00C9, "Bg_Spdweb"},
        {0x00CA, "En_Mt_tag"},
        {0x00CB, "Boss_01"},
        {0x00CC, "Boss_02"},
        {0x00CD, "Boss_03"},
        {0x00CE, "Boss_04"},
        {0x00CF, "Boss_05"},
        {0x00D0, "Boss_06"},
        {0x00D1, "Boss_07"},
        {0x00D2, "Bg_Dy_Yoseizo"},
        {0x00D3, "En_Boj_05"},
        {0x00D4, "En_Sob1"},
        {0x00D5, "En_Go"},
        {0x00D6, "En_Raf"},
        {0x00D7, "Obj_Funen"},
        {0x00D8, "Obj_Raillift"},
        {0x00D9, "Bg_Numa_Hana"},
        {0x00DA, "Obj_Flowerpot"},
        {0x00DB, "Obj_Spinyroll"},
        {0x00DC, "Dm_Hina"},
        {0x00DD, "En_Syateki_Wf"},
        {0x00DE, "Obj_Skateblock"},
        {0x00DF, "Obj_Iceblock"},
        {0x00E0, "En_Bigpamet"},
        {0x00E1, "En_Syateki_Dekunuts"},
        {0x00E2, "Elf_Msg3"},
        {0x00E3, "En_Fg"},
        {0x00E4, "Dm_Ravine"},
        {0x00E5, "Dm_Sa"},
        {0x00E6, "En_Slime"},
        {0x00E7, "En_Pr"},
        {0x00E8, "Obj_Toudai"},
        {0x00E9, "Obj_Entotu"},
        {0x00EA, "Obj_Bell"},
        {0x00EB, "En_Syateki_Okuta"},
        {0x00EC, "Obj_Shutter"},
        {0x00ED, "Dm_Zl"},
        {0x00EE, "En_Elfgrp"},
        {0x00EF, "Dm_Tsg"},
        {0x00F0, "En_Baguo"},
        {0x00F1, "Obj_Vspinyroll"},
        {0x00F2, "Obj_Smork"},
        {0x00F3, "En_Test2"},
        {0x00F4, "En_Test3"},
        {0x00F5, "En_Test4"},
        {0x00F6, "En_Bat"},
        {0x00F7, "En_Sekihi"},
        {0x00F8, "En_Wiz"},
        {0x00F9, "En_Wiz_Brock"},
        {0x00FA, "En_Wiz_Fire"},
        {0x00FB, "Eff_Change"},
        {0x00FC, "Dm_Statue"},
        {0x00FD, "Obj_Fireshield"},
        {0x00FE, "Bg_Ladder"},
        {0x00FF, "En_Mkk"},
        {0x0100, "Demo_Getitem"},
        {0x0101, "En_Dnb"},
        {0x0102, "En_Dnh"},
        {0x0103, "En_Dnk"},
        {0x0104, "En_Dnq"},
        {0x0105, "Bg_Keikoku_Saku"},
        {0x0106, "Obj_Hugebombiwa"},
        {0x0107, "En_Firefly2"},
        {0x0108, "En_Rat"},
        {0x0109, "En_Water_Effect"},
        {0x010A, "En_Kusa2"},
        {0x010B, "Bg_Spout_Fire"},
        {0x010C, "Bg_Dblue_Movebg"},
        {0x010D, "En_Dy_Extra"},
        {0x010E, "En_Bal"},
        {0x010F, "En_Ginko_Man"},
        {0x0110, "En_Warp_Uzu"},
        {0x0111, "Obj_Driftice"},
        {0x0112, "En_Look_Nuts"},
        {0x0113, "En_Mushi2"},
        {0x0114, "En_Fall"},
        {0x0115, "En_Mm3"},
        {0x0116, "Bg_Crace_Movebg"},
        {0x0117, "En_Dno"},
        {0x0118, "En_Pr2"},
        {0x0119, "En_Prz"},
        {0x011A, "En_Jso2"},
        {0x011B, "Obj_Etcetera"},
        {0x011C, "En_Egol"},
        {0x011D, "Obj_Mine"},
        {0x011E, "Obj_Purify"},
        {0x011F, "En_Tru"},
        {0x0120, "En_Trt"},
        {0x0121, "En_Test5"},
        {0x0122, "En_Test6"},
        {0x0123, "En_Az"},
        {0x0124, "En_Estone"},
        {0x0125, "Bg_Hakugin_Post"},
        {0x0126, "Dm_Opstage"},
        {0x0127, "Dm_Stk"},
        {0x0128, "Dm_Char00"},
        {0x0129, "Dm_Char01"},
        {0x012A, "Dm_Char02"},
        {0x012B, "Dm_Char03"},
        {0x012C, "Dm_Char04"},
        {0x012D, "Dm_Char05"},
        {0x012E, "Dm_Char06"},
        {0x012F, "Dm_Char07"},
        {0x0130, "Dm_Char08"},
        {0x0131, "Dm_Char09"},
        {0x0132, "Obj_Tokeidai"},
        {0x0133, "En_Mnk"},
        {0x0134, "En_Egblock"},
        {0x0135, "En_Guard_Nuts"},
        {0x0136, "Bg_Hakugin_Bombwall"},
        {0x0137, "Obj_Tokei_Tobira"},
        {0x0138, "Bg_Hakugin_Elvpole"},
        {0x0139, "En_Ma4"},
        {0x013A, "En_Twig"},
        {0x013B, "En_Po_Fusen"},
        {0x013C, "En_Door_Etc"},
        {0x013D, "En_Bigokuta"},
        {0x013E, "Bg_Icefloe"},
        {0x013F, "Obj_Ocarinalift"},
        {0x0140, "En_Time_Tag"},
        {0x0141, "Bg_Open_Shutter"},
        {0x0142, "Bg_Open_Spot"},
        {0x0143, "Bg_Fu_Kaiten"},
        {0x0144, "Obj_Aqua"},
        {0x0145, "En_Elforg"},
        {0x0146, "En_Elfbub"},
        {0x0147, "En_Fu_Mato"},
        {0x0148, "En_Fu_Kago"},
        {0x0149, "En_Osn"},
        {0x014A, "Bg_Ctower_Gear"},
        {0x014B, "En_Trt2"},
        {0x014C, "Obj_Tokei_Step"},
        {0x014D, "Bg_Lotus"},
        {0x014E, "En_Kame"},
        {0x014F, "Obj_Takaraya_Wall"},
        {0x0150, "Bg_Fu_Mizu"},
        {0x0151, "En_Sellnuts"},
        {0x0152, "Bg_Dkjail_Ivy"},
        {0x0153, "Obj_Visiblock"},
        {0x0154, "En_Takaraya"},
        {0x0155, "En_Tsn"},
        {0x0156, "En_Ds2n"},
        {0x0157, "En_Fsn"},
        {0x0158, "En_Shn"},
        {0x0159, "En_Stop_heishi"},
        {0x015A, "Obj_Bigicicle"},
        {0x015B, "En_Lift_Nuts"},
        {0x015C, "En_Tk"},
        {0x015D, "Bg_Market_Step"},
        {0x015E, "Obj_Lupygamelift"},
        {0x015F, "En_Test7"},
        {0x0160, "Obj_Lightblock"},
        {0x0161, "Mir_Ray2"},
        {0x0162, "En_Wdhand"},
        {0x0163, "En_Gamelupy"},
        {0x0164, "Bg_Danpei_Movebg"},
        {0x0165, "En_Snowwd"},
        {0x0166, "En_Pm"},
        {0x0167, "En_Gakufu"},
        {0x0168, "Elf_Msg4"},
        {0x0169, "Elf_Msg5"},
        {0x016A, "En_Col_Man"},
        {0x016B, "En_Talk_Gibud"},
        {0x016C, "En_Giant"},
        {0x016D, "Obj_Snowball"},
        {0x016E, "Boss_Hakugin"},
        {0x016F, "En_Gb2"},
        {0x0170, "En_Onpuman"},
        {0x0171, "Bg_Tobira01"},
        {0x0172, "En_Tag_Obj"},
        {0x0173, "Obj_Dhouse"},
        {0x0174, "Obj_Hakaisi"},
        {0x0175, "Bg_Hakugin_Switch"},
        {0x0176, "En_Snowman"},
        {0x0177, "TG_Sw"},
        {0x0178, "En_Po_Sisters"},
        {0x0179, "En_Pp"},
        {0x017A, "En_Hakurock"},
        {0x017B, "En_Hanabi"},
        {0x017C, "Obj_Dowsing"},
        {0x017D, "Obj_Wind"},
        {0x017E, "En_Racedog"},
        {0x017F, "En_Kendo_Js"},
        {0x0180, "Bg_Botihasira"},
        {0x0181, "En_Fish2"},
        {0x0182, "En_Pst"},
        {0x0183, "En_Poh"},
        {0x0184, "Obj_Spidertent"},
        {0x0185, "En_Zoraegg"},
        {0x0186, "En_Kbt"},
        {0x0187, "En_Gg"},
        {0x0188, "En_Maruta"},
        {0x0189, "Obj_Snowball2"},
        {0x018A, "En_Gg2"},
        {0x018B, "Obj_Ghaka"},
        {0x018C, "En_Dnp"},
        {0x018D, "En_Dai"},
        {0x018E, "Bg_Goron_Oyu"},
        {0x018F, "En_Kgy"},
        {0x0190, "En_Invadepoh"},
        {0x0191, "En_Gk"},
        {0x0192, "En_An"},
        {0x0193, "En_Bee"},
        {0x0194, "En_Ot"},
        {0x0195, "En_Dragon"},
        {0x0196, "Obj_Dora"},
        {0x0197, "En_Bigpo"},
        {0x0198, "Obj_Kendo_Kanban"},
        {0x0199, "Obj_Hariko"},
        {0x019A, "En_Sth"},
        {0x019B, "Bg_Sinkai_Kabe"},
        {0x019C, "Bg_Haka_Curtain"},
        {0x019D, "Bg_Kin2_Bombwall"},
        {0x019E, "Bg_Kin2_Fence"},
        {0x019F, "Bg_Kin2_Picture"},
        {0x01A0, "Bg_Kin2_Shelf"},
        {0x01A1, "En_Rail_Skb"},
        {0x01A2, "En_Jg"},
        {0x01A3, "En_Tru_Mt"},
        {0x01A4, "Obj_Um"},
        {0x01A5, "En_Neo_Reeba"},
        {0x01A6, "Bg_Mbar_Chair"},
        {0x01A7, "Bg_Ikana_Block"},
        {0x01A8, "Bg_Ikana_Mirror"},
        {0x01A9, "Bg_Ikana_Rotaryroom"},
        {0x01AA, "Bg_Dblue_Balance"},
        {0x01AB, "Bg_Dblue_Waterfall"},
        {0x01AC, "En_Kaizoku"},
        {0x01AD, "En_Ge2"},
        {0x01AE, "En_Ma_Yts"},
        {0x01AF, "En_Ma_Yto"},
        {0x01B0, "Obj_Tokei_Turret"},
        {0x01B1, "Bg_Dblue_Elevator"},
        {0x01B2, "Obj_Warpstone"},
        {0x01B3, "En_Zog"},
        {0x01B4, "Obj_Rotlift"},
        {0x01B5, "Obj_Jg_Gakki"},
        {0x01B6, "Bg_Inibs_Movebg"},
        {0x01B7, "En_Zot"},
        {0x01B8, "Obj_Tree"},
        {0x01B9, "Obj_Y2lift"},
        {0x01BA, "Obj_Y2shutter"},
        {0x01BB, "Obj_Boat"},
        {0x01BC, "Obj_Taru"},
        {0x01BD, "Obj_Hunsui"},
        {0x01BE, "En_Jc_Mato"},
        {0x01BF, "Mir_Ray3"},
        {0x01C0, "En_Zob"},
        {0x01C1, "Elf_Msg6"},
        {0x01C2, "Obj_Nozoki"},
        {0x01C3, "En_Toto"},
        {0x01C4, "En_Railgibud"},
        {0x01C5, "En_Baba"},
        {0x01C6, "En_Suttari"},
        {0x01C7, "En_Zod"},
        {0x01C8, "En_Kujiya"},
        {0x01C9, "En_Geg"},
        {0x01CA, "Obj_Kinoko"},
        {0x01CB, "Obj_Yasi"},
        {0x01CC, "En_Tanron1"},
        {0x01CD, "En_Tanron2"},
        {0x01CE, "En_Tanron3"},
        {0x01CF, "Obj_Chan"},
        {0x01D0, "En_Zos"},
        {0x01D1, "En_S_Goro"},
        {0x01D2, "En_Nb"},
        {0x01D3, "En_Ja"},
        {0x01D4, "Bg_F40_Block"},
        {0x01D5, "Bg_F40_Switch"},
        {0x01D6, "En_Po_Composer"},
        {0x01D7, "En_Guruguru"},
        {0x01D8, "Oceff_Wipe5"},
        {0x01D9, "En_Stone_heishi"},
        {0x01DA, "Oceff_Wipe6"},
        {0x01DB, "En_Scopenuts"},
        {0x01DC, "En_Scopecrow"},
        {0x01DD, "Oceff_Wipe7"},
        {0x01DE, "Eff_Kamejima_Wave"},
        {0x01DF, "En_Hg"},
        {0x01E0, "En_Hgo"},
        {0x01E1, "En_Zov"},
        {0x01E2, "En_Ah"},
        {0x01E3, "Obj_Hgdoor"},
        {0x01E4, "Bg_Ikana_Bombwall"},
        {0x01E5, "Bg_Ikana_Ray"},
        {0x01E6, "Bg_Ikana_Shutter"},
        {0x01E7, "Bg_Haka_Bombwall"},
        {0x01E8, "Bg_Haka_Tomb"},
        {0x01E9, "En_Sc_Ruppe"},
        {0x01EA, "Bg_Iknv_Doukutu"},
        {0x01EB, "Bg_Iknv_Obj"},
        {0x01EC, "En_Pamera"},
        {0x01ED, "Obj_HsStump"},
        {0x01EE, "En_Hidden_Nuts"},
        {0x01EF, "En_Zow"},
        {0x01F0, "En_Talk"},
        {0x01F1, "En_Al"},
        {0x01F2, "En_Tab"},
        {0x01F3, "En_Nimotsu"},
        {0x01F4, "En_Hit_Tag"},
        {0x01F5, "En_Ruppecrow"},
        {0x01F6, "En_Tanron4"},
        {0x01F7, "En_Tanron5"},
        {0x01F8, "En_Tanron6"},
        {0x01F9, "En_Daiku2"},
        {0x01FA, "En_Muto"},
        {0x01FB, "En_Baisen"},
        {0x01FC, "En_Heishi"},
        {0x01FD, "En_Demo_heishi"},
        {0x01FE, "En_Dt"},
        {0x01FF, "En_Cha"},
        {0x0200, "Obj_Dinner"},
        {0x0201, "Eff_Lastday"},
        {0x0202, "Bg_Ikana_Dharma"},
        {0x0203, "En_Akindonuts"},
        {0x0204, "Eff_Stk"},
        {0x0205, "En_Ig"},
        {0x0206, "En_Rg"},
        {0x0207, "En_Osk"},
        {0x0208, "En_Sth2"},
        {0x0209, "En_Yb"},
        {0x020A, "En_Rz"},
        {0x020B, "En_Scopecoin"},
        {0x020C, "En_Bjt"},
        {0x020D, "En_Bomjima"},
        {0x020E, "En_Bomjimb"},
        {0x020F, "En_Bombers"},
        {0x0210, "En_Bombers2"},
        {0x0211, "En_Bombal"},
        {0x0212, "Obj_Moon_Stone"},
        {0x0213, "Obj_Mu_Pict"},
        {0x0214, "Bg_Ikninside"},
        {0x0215, "Eff_Zoraband"},
        {0x0216, "Obj_Kepn_Koya"},
        {0x0217, "Obj_Usiyane"},
        {0x0218, "En_Nnh"},
        {0x0219, "Obj_Kzsaku"},
        {0x021A, "Obj_Milk_Bin"},
        {0x021B, "En_Kitan"},
        {0x021C, "Bg_Astr_Bombwall"},
        {0x021D, "Bg_Iknin_Susceil"},
        {0x021E, "En_Bsb"},
        {0x021F, "En_Recepgirl"},
        {0x0220, "En_Thiefbird"},
        {0x0221, "En_Jgame_Tsn"},
        {0x0222, "Obj_Jgame_Light"},
        {0x0223, "Obj_Yado"},
        {0x0224, "Demo_Syoten"},
        {0x0225, "Demo_Moonend"},
        {0x0226, "Bg_Lbfshot"},
        {0x0227, "Bg_Last_Bwall"},
        {0x0228, "En_And"},
        {0x0229, "En_Invadepoh_Demo"},
        {0x022A, "Obj_Danpeilift"},
        {0x022B, "En_Fall2"},
        {0x022C, "Dm_Al"},
        {0x022D, "Dm_An"},
        {0x022E, "Dm_Ah"},
        {0x022F, "Dm_Nb"},
        {0x0230, "En_Drs"},
        {0x0231, "En_Ending_Hero"},
        {0x0232, "Dm_Bal"},
        {0x0233, "En_Paper"},
        {0x0234, "En_Hint_Skb"},
        {0x0235, "Dm_Tag"},
        {0x0236, "En_Bh"},
        {0x0237, "En_Ending_Hero2"},
        {0x0238, "En_Ending_Hero3"},
        {0x0239, "En_Ending_Hero4"},
        {0x023A, "En_Ending_Hero5"},
        {0x023B, "En_Ending_Hero6"},
        {0x023C, "Dm_Gm"},
        {0x023D, "Obj_Swprize"},
        {0x023E, "En_Invisible_Ruppe"},
        {0x023F, "Obj_Ending"},
        {0x0240, "En_Rsn"},
        {0x0241, "En_Hintstone"},
        {0x0242, "Elf_Msg7"},
        {0x0243, "Bg_Sea_Railings"},
        {0x0244, "Obj_Sea_Icepoint"},
        {0x0245, ""},
        {0x0246, "Fishing_"},
        {0x0247, "En_Po*"},
        {0x0248, "Bg_Sea_Bs_Object"},
        {0x0249, "Bg_Inibs_*"},
        {0x024A, "En_Tanron7"},
        {0x024B, "En_Inibs_*"},
        {0x024C, "Bg_Inibs_*"},
        {0x024D, "Fish"},
        {0x024E, "Dm_*"},
        {0x024F, "*"},
        {0x0250, "Obj_*"},
        {0x0251, "Obj_Lightveil"},
        {0x0252, "En_Test8"},
    };
    const auto it = names.find(static_cast<int>(actor.id));
    if (it == names.end())
        return nullptr;
    return it->second;
}

u32 PredictOwlAddress(game::Allocator& allocator) {
    // Allocations that are performed when loading the central Deku Palace room

    const auto alloc = [&](u32 size) { return allocator.Alloc(size, nullptr); };

    auto x = alloc(156);
    allocator.Free(x);
    alloc(76);
    alloc(516);
    alloc(1436);
    alloc(516);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(2228);
    alloc(1436);
    alloc(2228);
    alloc(1436);
    alloc(560);
    alloc(1436);
    alloc(560);
    alloc(1436);
    alloc(560);
    alloc(1436);
    alloc(1812);
    alloc(1436);
    alloc(1812);
    alloc(1436);
    alloc(1812);
    alloc(1436);
    alloc(1224);
    alloc(1436);
    alloc(740);
    alloc(1436);
    alloc(740);
    alloc(1436);
    alloc(740);
    alloc(1436);
    alloc(740);
    alloc(1436);
    alloc(740);
    alloc(1436);
    alloc(600);
    alloc(1436);
    alloc(600);
    alloc(1436);
    alloc(600);
    alloc(1436);
    alloc(600);
    alloc(1436);
    alloc(536);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(544);
    alloc(1436);
    alloc(3672);
    alloc(1436);
    alloc(720);
    alloc(1436);
    alloc(720);
    alloc(1436);
    alloc(512);
    alloc(1436);
    alloc(3028);
    alloc(1436);
    alloc(524);
    alloc(1436);
    alloc(768);
    alloc(1436);
    alloc(44);
    alloc(516);
    alloc(44);
    auto owl = alloc(656);
    alloc(1436);
    alloc(608);
    alloc(1436);

    return owl.addr;
}

template <typename... Args>
QString Format(const char* format, const Args&... args) {
    return QString::fromStdString(fmt::format(format, args...));
}

class ZeldaDialog;

struct ZeldaInfo {
    using Blocks = std::vector<std::pair<Ptr<game::AllocatorBlock>, game::AllocatorBlock>>;

    u32 HeapSize() const {
        return heap_end - heap_start;
    }

    int FindContainingHeapIdx(VAddr addr) {
        // Because there are many allocations, it's worth doing a binary search instead of a
        // naïve linear search.
        int a = 0;
        int b = blocks.size() - 1;
        while (a <= b) {
            const auto m = (a + b) / 2;
            const auto block_begin = blocks[m].first.addr;
            const auto block_end = block_begin + std::abs(blocks[m].second.size);

            if (block_begin <= addr && addr < block_end)
                return m;

            if (addr < block_begin)
                b = m - 1;
            else
                a = m + 1;
        }
        return -1;
    }

    constexpr auto Fields() const {
        return std::tie(heap_start, heap_end, blocks);
    }

    bool operator==(const ZeldaInfo& rhs) const {
        return Fields() == rhs.Fields();
    }

    bool operator!=(const ZeldaInfo& rhs) const {
        return !(*this == rhs);
    }

    u32 heap_start = 0;
    u32 heap_end = 1;
    Blocks blocks;
    u32 total_free_size = 0;
    Ptr<game::Player> player_actor = nullptr;
    Ptr<game::Actor> target_actor = nullptr;

    Ptr<game::Actor> player_attached_actor = nullptr;

    u32 predicted_owl_addr = 0;
};

class InfoHolder {
public:
    void SetInfo(std::shared_ptr<ZeldaInfo> info) {
        m_info = std::move(info);
    }

protected:
    std::shared_ptr<ZeldaInfo> m_info = std::make_unique<ZeldaInfo>();
};

constexpr const char* HeapTableColumns[] = {"Block", "Address", "Size",
                                            "State", "Ticks",   "Description"};

enum class HeapTableColumn {
    Block = 0,
    Address,
    Size,
    State,
    Ticks,
    Description,
};
static_assert(int(HeapTableColumn::Description) + 1 == std::size(HeapTableColumns));

class HeapTableModel : public QAbstractTableModel, public InfoHolder {
public:
    explicit HeapTableModel(ZeldaDialog* parent);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void BeginReset() {
        beginResetModel();
    }

    void EndReset() {
        endResetModel();
    }

private:
    ZeldaDialog* m_parent;
};

class HeapTableFilterModel : public QSortFilterProxyModel {
public:
    explicit HeapTableFilterModel(ZeldaDialog* parent);
    void InvalidateFilter() {
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

private:
    ZeldaDialog* m_parent;
};

class HeapViewWidget : public QOpenGLWidget, public InfoHolder {
public:
    explicit HeapViewWidget(ZeldaDialog* parent);
    void paintEvent(QPaintEvent*) override;
    bool event(QEvent*) override;

private:
    ZeldaDialog* m_parent;
};

class ZeldaDialog : public QDialog {
public:
    explicit ZeldaDialog(QWidget* parent = nullptr) : QDialog(parent) {
        resize(1900, 370);
        setWindowTitle(QStringLiteral("MM3D Heap Viewer"));
        setWindowFlag(Qt::WindowMinMaxButtonsHint, true);
        setWindowFlag(Qt::WindowMaximizeButtonHint, true);
        setWindowFlag(Qt::WindowStaysOnTopHint, false);

        m_heap_view = new HeapViewWidget(this);
        m_heap_view->setFixedHeight(30);

        m_heap_label = new QLabel(QStringLiteral("-"), this);
        m_free_heap_label = new QLabel(QStringLiteral("-"), this);

        m_heap_table_model = new HeapTableModel(this);
        m_heap_table_proxy_model = new HeapTableFilterModel(this);
        m_heap_table_proxy_model->setSourceModel(m_heap_table_model);
        m_heap_table_proxy_model->setFilterKeyColumn(-1);
        m_heap_table_proxy_model->setSortRole(Qt::UserRole);

        m_keep_selection_cbox = new QCheckBox(QStringLiteral("Keep selection"));
        m_show_actors_cbox = new QCheckBox(QStringLiteral("Show actors"));
        m_show_layouts_cbox = new QCheckBox(QStringLiteral("Show layouts"));
        m_show_free_cbox = new QCheckBox(QStringLiteral("Show free blocks"));
        m_show_other_cbox = new QCheckBox(QStringLiteral("Show others"));
        m_restrict_to_range_cbox = new QCheckBox(QStringLiteral("Restrict to range:"));
        m_range_start_ledit = new QLineEdit;
        m_range_end_ledit = new QLineEdit;
        m_cursor_label = new QLabel(QStringLiteral("-"), this);
        m_pause_time_cbox = new QCheckBox(QStringLiteral("Pause time"));
        auto* dump_btn = new QPushButton(QStringLiteral("Dump heap"));
        auto* copy_btn = new QPushButton(QStringLiteral("Copy heap"));
        auto* time_6pm_btn = new QPushButton(QStringLiteral("6PM"));

        m_heap_table_view = new QTableView(this);
        m_heap_table_view->setModel(m_heap_table_proxy_model);
        m_heap_table_view->verticalHeader()->hide();
        m_heap_table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_heap_table_view->setSelectionMode(QAbstractItemView::SingleSelection);
        m_heap_table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_heap_table_view->horizontalHeader()->setStretchLastSection(true);
        m_heap_table_view->setSortingEnabled(true);
        m_heap_table_view->sortByColumn(int(HeapTableColumn::Address),
                                        Qt::SortOrder::AscendingOrder);

        auto* heap_table_search = new SearchBar(this);
        heap_table_search->AddFindShortcut(this);
        heap_table_search->ConnectToFilterModel(m_heap_table_proxy_model);
        heap_table_search->hide();

        m_connected_actor_info_label = new QLabel(this);
        m_target_actor_info_label = new QLabel(this);

        auto* label_grid = new QGridLayout;
        label_grid->addWidget(m_heap_label, 0, 0);
        label_grid->addWidget(m_free_heap_label, 1, 0);
        label_grid->addWidget(m_connected_actor_info_label, 0, 1);
        label_grid->addWidget(m_target_actor_info_label, 1, 1);

        // Set up layouts

        auto* options = new QHBoxLayout;
        options->addWidget(m_keep_selection_cbox);
        options->addWidget(m_show_actors_cbox);
        options->addWidget(m_show_layouts_cbox);
        options->addWidget(m_show_free_cbox);
        options->addWidget(m_show_other_cbox);
        options->addWidget(m_restrict_to_range_cbox);
        options->addWidget(m_range_start_ledit);
        options->addWidget(new QLabel(QStringLiteral("-")));
        options->addWidget(m_range_end_ledit);
        options->addStretch();
        options->addWidget(m_cursor_label);
        options->addWidget(m_pause_time_cbox);
        options->addWidget(time_6pm_btn);
        options->addWidget(dump_btn);
        options->addWidget(copy_btn);

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(m_heap_view);
        layout->addLayout(label_grid);
        layout->addLayout(options);
        layout->addWidget(m_heap_table_view, 9);
        layout->addWidget(heap_table_search);
        setLayout(layout);

        connect(m_heap_table_view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                &ZeldaDialog::OnHeapTableSelectionChanged);

        connect(dump_btn, &QPushButton::pressed, this, [this] {
            if (!Core::System::GetInstance().IsPoweredOn())
                return;

            const QString path =
                QFileDialog::getSaveFileName(this, tr("Save File"), {}, tr("Binary files (*.bin)"));
            if (path.isEmpty())
                return;
            FileUtil::IOFile file(path.toStdString(), "wb");
            auto* heap = Core::System::GetInstance().Memory().GetPointer(m_info->heap_start);
            file.WriteBytes(heap, m_info->HeapSize());

            FileUtil::IOFile file2(path.toStdString() + ".allocator", "wb");
            file2.WriteBytes(&game::Allocator::Instance(), sizeof(game::Allocator));
        });

        connect(copy_btn, &QPushButton::pressed, this, [this] {
            if (!Core::System::GetInstance().IsPoweredOn())
                return;

            game::Allocator::Instance().CopyDebugInfo();
        });

        connect(time_6pm_btn, &QPushButton::pressed, [] {
            if (Core::System::GetInstance().IsPoweredOn())
                game::GetCommonData()->save.time = game::HhmmToTime(18, 0);
        });

        InitFilterControls();
    }

    // Called from the core. Do not touch widgets here.
    void Update() {
        if (!Core::System::GetInstance().IsPoweredOn())
            return;

        auto& allocator = game::Allocator::Instance();

        auto info = std::make_unique<ZeldaInfo>();
        info->heap_start = allocator.root_block.addr;
        info->heap_end = allocator.root_block_end.addr;

        info->blocks.reserve(allocator.block_count);
        const Ptr<game::AllocatorBlock> root_block = allocator.root_block;
        Ptr<game::AllocatorBlock> block = root_block;
        do {
            if (block->size > 0)
                info->total_free_size += block->size;

            Ptr<game::Actor> actor = GetActorFromBlock(block);
            if (actor) {
                if (actor->id == game::Id::Player)
                    info->player_actor = actor.Cast<game::Player>();
                else if (actor->id == game::Id::ObjOwlStatue)
                    info->target_actor = actor;
            }

            info->blocks.emplace_back(block, *block);
            block = block->next;
        } while (block != root_block);

        if (info->player_actor) {
            info->player_attached_actor = info->player_actor->attached_actor;

            // Ensure that we do not modify allocator state while an allocation or deallocation
            // is being performed.
            // Otherwise, the internal structures could be in an inconsistent state.

            if (allocator.crit_section->lock_count == 0) {
                game::Allocator backup = allocator;

                info->predicted_owl_addr = PredictOwlAddress(allocator);

                // Restore block and allocator state.
                for (const auto& [addr, block] : info->blocks) {
                    std::memcpy(addr.get(), &block, sizeof(block));
                }
                allocator = backup;
            }
        }

        // Update widgets on the proper thread.
        QMetaObject::invokeMethod(
            this, [this, info = std::move(info)]() mutable { UpdateUi(std::move(info)); });
    }

    void OnHeapViewReleaseEvent(QMouseEvent* e) {
        const auto pt = e->localPos();
        if (m_info->heap_start == 0)
            return;

        const int idx = m_info->FindContainingHeapIdx(TranslateMousePosToAddr(*m_info, pt.x()));
        if (idx == -1)
            return;

        m_heap_table_view->selectRow(SourceToProxyIdx(idx));
    }

    void OnHeapViewHoverEvent(QHoverEvent* e) {
        const auto pt = e->posF();
        if (pt.x() == -1 || m_info->heap_start == 0) {
            m_cursor_label->setText(QStringLiteral("-"));
            return;
        }
        m_cursor_label->setText(QStringLiteral("0x%1").arg(TranslateMousePosToAddr(*m_info, pt.x()),
                                                           8, 16, QLatin1Char('0')));
    }

    u32 TranslateMousePosToAddr(const ZeldaInfo& info, qreal x) const {
        return u32(info.heap_start + info.HeapSize() * x / m_heap_view->width());
    }

    std::optional<QModelIndex> GetSelectedIdx() const {
        auto smodel = m_heap_table_view->selectionModel();
        if (!smodel->hasSelection())
            return {};
        return m_heap_table_proxy_model->mapToSource(smodel->selectedRows()[0]);
    }

    bool ShouldShowActors() const {
        return m_show_actors_cbox->isChecked();
    }

    bool ShouldShowLayouts() const {
        return m_show_layouts_cbox->isChecked();
    }

    bool ShouldShowFree() const {
        return m_show_free_cbox->isChecked();
    }

    bool ShouldShowOthers() const {
        return m_show_other_cbox->isChecked();
    }

    std::pair<u32, u32> GetFilterRange() const {
        if (!m_restrict_to_range_cbox->isChecked())
            return {0, 0xFFFFFFFF};
        return {m_filter_range_start, m_filter_range_end};
    }

    u32 m_selected_addr = 0;
    u32 m_selected_size = 0;

private:
    void InitFilterControls() {
        for (auto* cbox :
             {m_show_actors_cbox, m_show_layouts_cbox, m_show_free_cbox, m_show_other_cbox}) {
            cbox->setChecked(true);
            connect(cbox, &QCheckBox::stateChanged, this,
                    [this](int state) { m_heap_table_proxy_model->InvalidateFilter(); });
        }

        const QFont fixed_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

        for (auto* ledit : {m_range_start_ledit, m_range_end_ledit}) {
            ledit->setClearButtonEnabled(true);
            ledit->setMaximumWidth(ledit->width() / 4);
            ledit->setFont(fixed_font);
        }

        m_range_end_ledit->setText(QStringLiteral("0xFFFFFFFF"));

        connect(m_range_start_ledit, &QLineEdit::textChanged, this, [this](const QString& text) {
            m_filter_range_start = text.toULong(nullptr, 16);
            m_heap_table_proxy_model->InvalidateFilter();
        });
        connect(m_range_end_ledit, &QLineEdit::textChanged, this, [this](const QString& text) {
            m_filter_range_end = text.toULong(nullptr, 16);
            m_heap_table_proxy_model->InvalidateFilter();
        });

        const auto restrict_toggled = [this](int state) {
            m_heap_table_proxy_model->InvalidateFilter();
            m_range_start_ledit->setEnabled(state == Qt::CheckState::Checked);
            m_range_end_ledit->setEnabled(state == Qt::CheckState::Checked);
        };
        restrict_toggled(m_restrict_to_range_cbox->checkState());
        connect(m_restrict_to_range_cbox, &QCheckBox::stateChanged, this, restrict_toggled);
    }

    void UpdateUi(std::unique_ptr<ZeldaInfo> info) {
        if (!Core::System::GetInstance().IsPoweredOn())
            return;

        static std::optional<u16> s_previous_time;
        if (m_pause_time_cbox->isChecked() && s_previous_time) {
            game::GetCommonData()->save.time = *s_previous_time;
        }
        s_previous_time = game::GetCommonData()->save.time;

        auto& allocator = game::Allocator::Instance();

        const bool heap_info_changed = *m_info != *info;
        if (heap_info_changed) {
            const u32 selected_addr = m_selected_addr;

            m_heap_table_model->BeginReset();

            m_info = std::move(info);
            m_heap_table_model->SetInfo(m_info);
            m_heap_view->SetInfo(m_info);

            m_heap_table_model->EndReset();

            m_heap_label->setText(Format("Root block: 0x{:08x} → 0x{:08x} ({} bytes) - {} blocks",
                                         allocator.root_block.addr, allocator.root_block_end.addr,
                                         allocator.size, allocator.block_count));

            if (selected_addr != 0 && m_keep_selection_cbox->isChecked()) {
                const int new_selected_idx = m_info->FindContainingHeapIdx(selected_addr);
                if (new_selected_idx != -1)
                    m_heap_table_view->selectRow(SourceToProxyIdx(new_selected_idx));
            }
        }

        m_free_heap_label->setText(Format("Free size: {} bytes / {} bytes ({:.3f}% free)",
                                          m_info->total_free_size, allocator.size,
                                          100.0f * m_info->total_free_size / allocator.size));

        m_heap_view->update();

        if (m_info->player_actor) {
            const Ptr<game::Player> player = m_info->player_actor;
            m_connected_actor_info_label->setText(Format(
                "Player: {:.3f} {:.3f} {:.3f} rot={:04x} | grabbable_actor={:08x} attached_actor={:08x}",
                player->pos.pos.x, player->pos.pos.y, player->pos.pos.z, static_cast<u16>(player->pos.rot.y),
                player->grabbable_actor.addr, player->attached_actor.addr));
        } else {
            m_connected_actor_info_label->clear();
        }

        m_target_actor_info_label->setDisabled(false);
        if (m_info->target_actor) {
            const Ptr<game::Actor> target = m_info->target_actor;
            const int statue_location = (target->params >> 4) & 0x1F;
            m_target_actor_info_label->setText(
                Format("Statue: {:08x} | params={:04x} (location={:02x})", target.addr, target->params, statue_location));
        } else {
            if (m_info->predicted_owl_addr != 0) {
                m_target_actor_info_label->setText(
                    Format("Statue: {:08x} (predicted load address)", m_info->predicted_owl_addr));
            } else {
                m_target_actor_info_label->setDisabled(true);
            }
        }
    }

    void OnHeapTableSelectionChanged(const QItemSelection& selected,
                                     const QItemSelection& deselected) {
        if (selected.indexes().size() != std::size(HeapTableColumns)) {
            m_selected_addr = 0;
            m_selected_size = 0;
            return;
        }
        const auto idx = GetSelectedIdx().value();
        const auto& [addr, block] = m_info->blocks[idx.row()];
        m_selected_addr = addr.addr;
        m_selected_size = std::abs(block.size);
        m_heap_view->update();
    }

    int SourceToProxyIdx(int idx) const {
        return m_heap_table_proxy_model->mapFromSource(m_heap_table_model->index(idx, 0)).row();
    }

    // Only for use on the UI thread.
    std::shared_ptr<ZeldaInfo> m_info = std::make_unique<ZeldaInfo>();

    QTableView* m_heap_table_view = nullptr;
    HeapTableModel* m_heap_table_model = nullptr;
    HeapTableFilterModel* m_heap_table_proxy_model = nullptr;

    HeapViewWidget* m_heap_view = nullptr;
    QLabel* m_heap_label = nullptr;
    QLabel* m_free_heap_label = nullptr;

    QCheckBox* m_keep_selection_cbox = nullptr;
    QCheckBox* m_show_actors_cbox = nullptr;
    QCheckBox* m_show_layouts_cbox = nullptr;
    QCheckBox* m_show_free_cbox = nullptr;
    QCheckBox* m_show_other_cbox = nullptr;
    QCheckBox* m_restrict_to_range_cbox = nullptr;
    QLineEdit* m_range_start_ledit = nullptr;
    QLineEdit* m_range_end_ledit = nullptr;
    QLabel* m_cursor_label = nullptr;
    QCheckBox* m_pause_time_cbox = nullptr;

    QLabel* m_connected_actor_info_label = nullptr;
    QLabel* m_target_actor_info_label = nullptr;

    u32 m_filter_range_start = 0;
    u32 m_filter_range_end = 0xFFFFFFFF;
};

HeapTableModel::HeapTableModel(ZeldaDialog* parent)
    : QAbstractTableModel(parent), m_parent(parent) {}

int HeapTableModel::rowCount(const QModelIndex& parent) const {
    return m_info->blocks.size();
}

int HeapTableModel::columnCount(const QModelIndex& parent) const {
    return std::size(HeapTableColumns);
}

QVariant HeapTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Orientation::Horizontal)
        return {};
    if (size_t(section) >= std::size(HeapTableColumns))
        return {};
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
        return {};
    return QString::fromLatin1(HeapTableColumns[section]);
}

enum HeapTableModelUserRole {
    HeapTableModelUserRole_Data = Qt::UserRole,
    HeapTableModelUserRole_IsActor,
    HeapTableModelUserRole_IsFree,
    HeapTableModelUserRole_IsInFilterRange,
};

QVariant HeapTableModel::data(const QModelIndex& index, int role) const {
    const auto& info = m_info;
    if (size_t(index.row()) >= info->blocks.size())
        return {};

    const auto& [addr, block] = info->blocks[index.row()];

    switch (role) {
    case HeapTableModelUserRole_IsActor:
        return IsActor(block);
    case HeapTableModelUserRole_IsFree:
        return block.IsFree() ||
               (block.flags & game::AllocatorBlock::Flag_IsRefCounted && block.ref_count == 0);
    case HeapTableModelUserRole_IsInFilterRange: {
        const auto [range_start, range_end] = m_parent->GetFilterRange();
        const u32 block_end_addr = addr.addr + std::abs(block.size);
        return (range_start <= addr.addr && addr.addr <= range_end) ||
               (range_start <= block_end_addr && block_end_addr <= range_end);
    }
    default:
        break;
    }

    switch (HeapTableColumn(index.column())) {
    case HeapTableColumn::Block:
        switch (role) {
        case Qt::DisplayRole:
            return QStringLiteral("0x%1").arg(addr.addr, 8, 16, QLatin1Char('0'));
        case Qt::UserRole:
            return addr.addr;
        default:
            return {};
        }
    case HeapTableColumn::Address:
        switch (role) {
        case Qt::DisplayRole:
            return QStringLiteral("0x%1").arg(addr.addr + sizeof(game::AllocatorBlock), 8, 16,
                                              QLatin1Char('0'));
        case Qt::UserRole:
            return addr.addr + u32(sizeof(game::AllocatorBlock));
        default:
            return {};
        }
    case HeapTableColumn::Size:
        switch (role) {
        case Qt::DisplayRole:
            return QStringLiteral("0x%1").arg(std::abs(block.size), 8, 16, QLatin1Char('0'));
        case Qt::UserRole:
            return std::abs(block.size);
        default:
            return {};
        }
    case HeapTableColumn::State:
        switch (role) {
        case Qt::DisplayRole:
            return block.IsFree() ? QStringLiteral("Free") : QStringLiteral("Used");
        case Qt::UserRole:
            return block.IsFree() ? 1 : 0;
        default:
            return {};
        }
    case HeapTableColumn::Ticks:
        if (block.IsFree())
            return {};
        switch (role) {
        case Qt::DisplayRole:
            return QStringLiteral("%1").arg(block.alloc_ticks, 16, 16, QLatin1Char('0'));
        case Qt::UserRole:
            return quint64(block.alloc_ticks);
        default:
            return {};
        }
    case HeapTableColumn::Description:
        if (block.IsFree())
            return {};
        switch (role) {
        case Qt::DisplayRole:
            if (block.flags & game::AllocatorBlock::Flag_IsRefCounted)
                return QStringLiteral("(ref counted: %1)").arg(block.ref_count);
            [[fallthrough]];
        case Qt::UserRole:
            if (!block.name)
                return {};
            if (const auto actor = GetActorFromBlock(addr)) {
                if (const char* name = GetActorName(*actor)) {
                    return QStringLiteral("Actor: ID 0x%1 (%2)")
                        .arg(u16(actor->id), 4, 16, QLatin1Char('0'))
                        .arg(QString::fromLatin1(name));
                }
                return QStringLiteral("Actor: ID 0x%1")
                    .arg(u16(actor->id), 4, 16, QLatin1Char('0'));
            }
            return QString::fromLatin1(block.name);
        default:
            return {};
        }
    }

    return {};
}

HeapTableFilterModel::HeapTableFilterModel(ZeldaDialog* parent)
    : QSortFilterProxyModel(parent), m_parent(parent) {}

bool HeapTableFilterModel::filterAcceptsRow(int source_row,
                                            const QModelIndex& source_parent) const {
    const auto idx = sourceModel()->index(source_row, 0, source_parent);
    const bool is_actor = idx.data(HeapTableModelUserRole_IsActor).toBool();
    const bool is_layout = idx.siblingAtColumn(int(HeapTableColumn::Description))
                               .data(Qt::UserRole)
                               .toString()
                               .contains(QStringLiteral("Layout\\"));
    const bool is_free = idx.data(HeapTableModelUserRole_IsFree).toBool();
    const bool is_other = !is_actor && !is_layout && !is_free;

    if (!m_parent->ShouldShowActors() && is_actor)
        return false;
    if (!m_parent->ShouldShowLayouts() && is_layout)
        return false;
    if (!m_parent->ShouldShowFree() && is_free)
        return false;
    if (!m_parent->ShouldShowOthers() && is_other)
        return false;

    if (!idx.data(HeapTableModelUserRole_IsInFilterRange).toBool())
        return false;

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

HeapViewWidget::HeapViewWidget(ZeldaDialog* parent) : QOpenGLWidget(parent), m_parent(parent) {
    setAttribute(Qt::WA_Hover);
}

void HeapViewWidget::paintEvent(QPaintEvent*) {
    const auto& info = m_info;
    QPainter painter(this);
    const int width = info->HeapSize();
    const int height = 1;
    painter.setWindow(QRect(0, 0, width, height));

    // Background
    painter.fillRect(0, 0, width, height, QColor(0x212121));

    for (const auto& [addr, block] : info->blocks) {
        QColor color;
        if (block.size <= 0) {
            color.setRgb(0xe83e33);
            if (block.ref_count == 0)
                color.setRgb(0);
        } else {
            color.setRgb(0x71e858);
        }
        painter.fillRect(addr.addr - info->heap_start, 0, std::abs(block.size), height, color);
    }

    if (m_info->target_actor) {
        painter.fillRect(m_info->target_actor.addr - info->heap_start, 0, 0xC000u, height,
                         QColor(0xff, 0xff, 0xff));
    }

    if (m_info->player_actor) {
        Ptr<game::Actor> attached_actor = m_info->player_attached_actor;
        if (attached_actor) {
            painter.fillRect(attached_actor.addr - info->heap_start, 0, 0xC000u, height,
                             QColor(0xff, 0xf2, 0x36));
        }
        if (attached_actor.addr - 0x10 == m_info->target_actor.addr) {
            painter.fillRect(attached_actor.addr - info->heap_start, 0, 0xC000u, height,
                             QColor(0x0D, 0x9A, 0xff));
        }
    }

    if (m_parent->m_selected_addr != 0) {
        painter.fillRect(m_parent->m_selected_addr - info->heap_start, 0,
                         std::max(m_parent->m_selected_size, 0xC000u), height,
                         QColor(0xff, 0xff, 0xff, 0x70));
    }
}

bool HeapViewWidget::event(QEvent* e) {
    switch (e->type()) {
    case QEvent::MouseButtonRelease:
        m_parent->OnHeapViewReleaseEvent(static_cast<QMouseEvent*>(e));
        return true;
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        m_parent->OnHeapViewHoverEvent(static_cast<QHoverEvent*>(e));
        return true;
    default:
        break;
    }
    return QWidget::event(e);
}

static ZeldaDialog* s_dialog;

void OnFrame() {
    if (!s_dialog)
        return;
    s_dialog->Update();
}

} // namespace

void ShowWindow(QWidget* parent) {
    if (!s_dialog) {
        s_dialog = new ZeldaDialog(parent);
    }
    s_dialog->show();
    Core::System::GetInstance().SetFrameCallback(OnFrame);
}

} // namespace zelda

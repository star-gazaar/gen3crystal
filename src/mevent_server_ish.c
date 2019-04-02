#include "global.h"
#include "alloc.h"
#include "decompress.h"
#include "overworld.h"
#include "script.h"
#include "battle_tower.h"
#include "mevent.h"
#include "mystery_event_script.h"
#include "mevent_server_ish.h"

EWRAM_DATA struct mevent_srv_ish * s_mevent_srv_ish_ptr = NULL;

static void mevent_srv_ish_init(struct mevent_srv_ish *, u32, u32);
static u32 mevent_srv_ish_exec(struct mevent_srv_ish *);
static void mevent_srv_ish_free_resources(struct mevent_srv_ish *);

extern const struct mevent_cmd_ish gUnknown_082F2598[];

void mevent_srv_ish_do_init(u32 arg)
{
    s_mevent_srv_ish_ptr = AllocZeroed(sizeof(struct mevent_srv_ish));
    mevent_srv_ish_init(s_mevent_srv_ish_ptr, 1, 0);
    s_mevent_srv_ish_ptr->unk_4C = arg;
}

u32 mevent_srv_ish_do_exec(u16 * a0)
{
    u32 result;
    if (s_mevent_srv_ish_ptr == NULL)
        return 6;
    result = mevent_srv_ish_exec(s_mevent_srv_ish_ptr);
    if (result == 6)
    {
        *a0 = s_mevent_srv_ish_ptr->param;
        mevent_srv_ish_free_resources(s_mevent_srv_ish_ptr);
        Free(s_mevent_srv_ish_ptr);
        s_mevent_srv_ish_ptr = NULL;
    }
    return result;
}

void mevent_srv_ish_inc_flag(void)
{
    s_mevent_srv_ish_ptr->flag++;
}

void * mevent_srv_ish_get_buffer(void)
{
    return s_mevent_srv_ish_ptr->buffer;
}

void mevent_srv_ish_set_param(u32 a0)
{
    s_mevent_srv_ish_ptr->param = a0;
}

static void mevent_srv_ish_init(struct mevent_srv_ish * svr, u32 sendPlayerNo, u32 recvPlayerNo)
{
    svr->unk_00 = 0;
    svr->mainseqno = 0;
    svr->flag = 0;
    svr->sendBuffer = AllocZeroed(ME_SEND_BUF_SIZE);
    svr->recvBuffer = AllocZeroed(ME_SEND_BUF_SIZE);
    svr->cmdBuffer = AllocZeroed(ME_SEND_BUF_SIZE);
    svr->buffer = AllocZeroed(0x40);
    mevent_srv_sub_init(&svr->manager, sendPlayerNo, recvPlayerNo);
}

static void mevent_srv_ish_free_resources(struct mevent_srv_ish * svr)
{
    Free(svr->sendBuffer);
    Free(svr->recvBuffer);
    Free(svr->cmdBuffer);
    Free(svr->buffer);
}

static void mevent_srv_ish_jmp_buffer(struct mevent_srv_ish * svr)
{
    memcpy(svr->cmdBuffer, svr->recvBuffer, ME_SEND_BUF_SIZE);
    svr->cmdidx = 0;
}

static void mevent_srv_ish_send_word(struct mevent_srv_ish * svr, u32 ident, u32 word)
{
    CpuFill32(0, svr->sendBuffer, ME_SEND_BUF_SIZE);
    *(u32 *)svr->sendBuffer = word;
    mevent_srv_sub_init_send(&svr->manager, ident, svr->sendBuffer, sizeof(u32));
}

static u32 ish_mainseq_0(struct mevent_srv_ish * svr)
{
    // init
    memcpy(svr->cmdBuffer, gUnknown_082F2598, ME_SEND_BUF_SIZE);
    svr->cmdidx = 0;
    svr->mainseqno = 4;
    svr->flag = 0;
    return 0;
}

static u32 ish_mainseq_1(struct mevent_srv_ish * svr)
{
    // done
    return 6;
}


static u32 ish_mainseq_2(struct mevent_srv_ish * svr)
{
    // do recv
    if (mevent_srv_sub_recv(&svr->manager))
    {
        svr->mainseqno = 4;
        svr->flag = 0;
    }
    return 1;
}

static u32 ish_mainseq_3(struct mevent_srv_ish * svr)
{
    // do send
    if (mevent_srv_sub_send(&svr->manager))
    {
        svr->mainseqno = 4;
        svr->flag = 0;
    }
    return 1;
}

static u32 ish_mainseq_4(struct mevent_srv_ish * svr)
{
    // process command
    struct mevent_cmd_ish * cmd = &svr->cmdBuffer[svr->cmdidx];
    ++svr->cmdidx;
    switch (cmd->instr)
    {
    case 0:
        break;
    case 1:
        svr->param = cmd->parameter;
        svr->mainseqno = 1;
        svr->flag = 0;
        break;
    case 2:
        mevent_srv_sub_init_recv(&svr->manager, cmd->parameter, svr->recvBuffer);
        svr->mainseqno = 2;
        svr->flag = 0;
        break;
    case 3:
        svr->mainseqno = 3;
        svr->flag = 0;
        break;
    case 20:
        mevent_srv_sub_init_send(&svr->manager, 0x14, svr->sendBuffer, 0);
        svr->mainseqno = 3;
        svr->flag = 0;
        break;
    case 19:
        mevent_srv_ish_send_word(svr, 0x12, GetGameStat(cmd->parameter));
        svr->mainseqno = 3;
        svr->flag = 0;
        break;
    case 6:
        if (svr->param == 0)
            mevent_srv_ish_jmp_buffer(svr);
        break;
    case 7:
        if (svr->param == 1)
            mevent_srv_ish_jmp_buffer(svr);
        break;
    case 4:
        mevent_srv_ish_jmp_buffer(svr);
        break;
    case 5:
        memcpy(svr->buffer, svr->recvBuffer, 0x40);
        svr->mainseqno = 5;
        svr->flag = 0;
        return 2;
    case 11:
        memcpy(svr->buffer, svr->recvBuffer, 0x40);
        svr->mainseqno = 5;
        svr->flag = 0;
        return 3;
    case 12:
        memcpy(svr->buffer, svr->recvBuffer, 0x40);
        svr->mainseqno = 5;
        svr->flag = 0;
        return 5;
    case 13:
        svr->mainseqno = 5;
        svr->flag = 0;
        return 4;
    case 8:
        sub_801B580(svr->sendBuffer, svr->unk_4C);
        mevent_srv_sub_init_send(&svr->manager, 0x11, svr->sendBuffer, sizeof(struct MEventStruct_Unk1442CC));
        break;
    case 14:
        mevent_srv_ish_send_word(svr, 0x13, svr->param);
        break;
    case 10:
        sub_801B21C(svr->recvBuffer);
        break;
    case 9:
        if (!sub_801B1A4(svr->recvBuffer))
        {
            sub_801B078(svr->recvBuffer);
            mevent_srv_ish_send_word(svr, 0x13, 0);
        }
        else
            mevent_srv_ish_send_word(svr, 0x13, 1);
        break;
    case 15:
        svr->mainseqno = 6;
        svr->flag = 0;
        break;
    case 16:
        sub_801B508(svr->recvBuffer);
        break;
    case 17:
        InitRamScript_NoEventObject(svr->recvBuffer, 1000);
        break;
    case 18:
        memcpy(&gSaveBlock2Ptr->frontier.ereaderTrainer, svr->recvBuffer, 0xbc);
        ValidateEReaderTrainer();
        break;
    case 21:
        memcpy(gDecompressionBuffer, svr->recvBuffer, ME_SEND_BUF_SIZE);
        svr->mainseqno = 7;
        svr->flag = 0;
        break;
    }

    return 1;
}

static u32 ish_mainseq_5(struct mevent_srv_ish * svr)
{
    // wait flag
    if (svr->flag)
    {
        svr->mainseqno = 4;
        svr->flag = 0;
    }
    return 1;
}

static u32 ish_mainseq_6(struct mevent_srv_ish * svr)
{
    // ???
    switch (svr->flag)
    {
    case 0:
        sub_8153870(svr->recvBuffer);
        ++svr->flag;
        break;
    case 1:
        if (!sub_8153884(&svr->param))
        {
            svr->mainseqno = 4;
            svr->flag = 0;
        }
        break;
    }
    return 1;
}

static u32 ish_mainseq_7(struct mevent_srv_ish * svr)
{
    // exec arbitrary code
    u32 (*func)(u32 *, struct SaveBlock2 *, struct SaveBlock1 *) = (void *)gDecompressionBuffer;
    if (func(&svr->param, gSaveBlock2Ptr, gSaveBlock1Ptr) == 1)
    {
        svr->mainseqno = 4;
        svr->flag = 0;
    }
    return 1;
}

static u32 mevent_srv_ish_exec(struct mevent_srv_ish * svr)
{
    u32 (*funcs[])(struct mevent_srv_ish *) = {
        ish_mainseq_0,
        ish_mainseq_1,
        ish_mainseq_2,
        ish_mainseq_3,
        ish_mainseq_4,
        ish_mainseq_5,
        ish_mainseq_6,
        ish_mainseq_7
    };
    return funcs[svr->mainseqno](svr);
}

#define	IN_FELICALIB
#include "felicalib.h"
#include "felicaint.h"


/**
   @brief PaSoRi ���I�[�v������
   @param[in] dummy �_�~�[ (libpasori �Ƃ̌݊����̂���)
   @retval pasori �n���h��

   felica.dll �����[�h�A����������
 */
pasori *pasori_open(char *dummy)
{
    pasori *p;

    p = (pasori *)malloc(sizeof(pasori));
    
    /* open felica.dll */
    p->hInstDLL = LoadLibrary("C:\\Program Files\\Common Files\\Sony Shared\\FeliCaLibrary\\felica.dll");
    if (p->hInstDLL == NULL) {
	free(p);
	return NULL;
    }
	
    /* resolve function pointers */
#define resolve_entry(f) p->f = (f ## _t)GetProcAddress(p->hInstDLL, #f)
    resolve_entry(initialize_library);
    resolve_entry(dispose_library);
    resolve_entry(open_reader_writer_auto);
    resolve_entry(close_reader_writer);
    resolve_entry(polling_and_get_card_information);
    resolve_entry(polling_and_request_system_code);
    resolve_entry(polling_and_search_service_code);
    resolve_entry(read_block_without_encryption);

    if (!p->initialize_library()) {
	free(p);
	return NULL;
    }
    return p;
}

/**
   @brief PaSoRi �n���h�����N���[�Y����
   @param[in] pasori pasori�n���h��

   felica.dll ���������
*/
void pasori_close(pasori *p)
{
    p->dispose_library();
    FreeLibrary(p->hInstDLL);
}

/**
   @brief PaSoRi ������������
   @param[in] p pasori�n���h��
   @return �G���[�R�[�h
*/
int pasori_init(pasori *p)
{
    if (!p->open_reader_writer_auto()) {
	return -1;
    }
    return 0;
}

static felica *alloc_felica(pasori *p, uint16 systemcode)
{
    felica *f = (felica *)malloc(sizeof(felica));
    f->p = p;
    f->systemcode = H2NS(systemcode);

    return f;
}

/**
   @brief FeliCa ���|�[�����O����
   @param[in] p pasori�n���h��
   @param[in] systemcode �V�X�e���R�[�h
   @param[in] RFU RFU (�g�p���Ȃ�)
   @param[in] timeslot �^�C���X���b�g
   @return felica�n���h��
*/
felica *felica_polling(pasori *p, uint16 systemcode, uint8 RFU, uint8 timeslot)
{
    felica *f;
    POLLING polling;
    uint16 sc;
    uint8 number_of_cards = 0;
    CARD_INFO card_info;

    f = alloc_felica(p, systemcode);

    sc = H2NS(systemcode);
    polling.system_code = (uint8 *)&sc;
    polling.time_slot = timeslot;

    card_info.card_idm = f->IDm;
    card_info.card_pmm = f->PMm;

    if (!p->polling_and_get_card_information(&polling, &number_of_cards, &card_info) ||
	number_of_cards == 0) {
	free(f);
	return NULL;
    }

    return f;
}

/**
   @brief �Í�������Ă��Ȃ��u���b�N��ǂݍ���
   @param[in] f felica�n���h��
   @param[in] mode ���[�h(�g�p���Ȃ�)
   @param[in] addr �u���b�N�ԍ�
   @param[out] data �f�[�^(16�o�C�g)
   @return �G���[�R�[�h

   �T�[�r�X�R�[�h�A�u���b�N�ԍ����w�肵�ău���b�N��ǂݍ��ށB
   �V�X�e���R�[�h�� felica_polling �Ŏw�肵�����̂��g�p�����B
*/
int felica_read_without_encryption02(felica *f, int servicecode, int mode, uint8 addr, uint8 *data)
{
    INSTR_READ_BLOCK irb;
    OUTSTR_READ_BLOCK orb;

    uint8 service_code_list[2] = { servicecode & 0xff, servicecode >> 8 };
    uint8 block_list[2] = { 0x80, addr };
    uint8 status_flag1, status_flag2;
    uint8 result_number_of_blocks = 0;

    irb.card_idm = f->IDm;
    irb.number_of_services = 1;
    irb.service_code_list = service_code_list;
    irb.number_of_blocks = 1;
    irb.block_list = block_list;

    orb.status_flag_1 = &status_flag1;
    orb.status_flag_2 = &status_flag2;
    orb.result_number_of_blocks = &result_number_of_blocks;
    orb.block_data = data;

    if (!f->p->read_block_without_encryption(&irb, &orb)) {
	return -1;
    }
    if (status_flag1 != 0) {
	return -1;
    }
    return 0;
}

/*------------- ��������� libpasori �݊��łȂ� (�Ǝ�) ------------*/

/**
   @brief �V�X�e���R�[�h�̗�
   @param[in] p pasori�n���h��
   @return felica�n���h��

   ���ʂ́Afelica�\���̂� num_system_code/system_code �Ɋi�[�����B
   �Ȃ��A�V�X�e���R�[�h�̃G���f�B�A���͋t�Ɋi�[����Ă���̂Œ��ӂ��邱�ƁB
*/
felica * enum_systemcode(pasori *p)
{
    felica *f;
    POLLING polling;
    CARD_INFO card_info;
    INSTR_REQ_SYSTEM_CODE irs;
    OUTSTR_REQ_SYSTEM_CODE ors;

    f = alloc_felica(p, 0xffff);

    polling.system_code = (uint8 *)&f->systemcode;
    polling.time_slot = 0;

    card_info.card_idm = f->IDm;
    card_info.card_pmm = f->PMm;

    irs.card_idm = f->IDm;
    
    ors.system_code_list = (uint8 *)f->system_code;

    if (!f->p->polling_and_request_system_code(&polling, &irs, &card_info, &ors)) {
	free(f);
	return NULL;
    }

    f->num_system_code = ors.number_of_system_codes;
    return f;
}

/**
   @brief �T�[�r�X/�G���A�R�[�h�̗�
   @param[in] p pasori�n���h��
   @param[in] systemcode �V�X�e���R�[�h
   @return felica�n���h��

   ���ʂ́Afelica�\���̂� num_area_code/area_code/end_service_code �����
   num_service_code/service_code �Ɋi�[�����B
*/
felica * enum_service(pasori *p, uint16 systemcode)
{
    felica *f;

    POLLING polling;
    CARD_INFO card_info;
    INSTR_SEARCH_SERVICE iss;
    OUTSTR_SEARCH_SERVICE oss;

    f = alloc_felica(p, systemcode);

    polling.system_code = (uint8 *)&f->systemcode;
    polling.time_slot = 0;

    card_info.card_idm = f->IDm;
    card_info.card_pmm = f->PMm;

    iss.buffer_size_of_area_codes = MAX_AREA_CODE;
    iss.buffer_size_of_service_codes = MAX_SERVICE_CODE;
    iss.offset_of_area_service_index = 0;

    oss.num_area_codes = 10;
    oss.num_service_codes = 10;
    oss.service_code_list     = (uint8 *)f->service_code;
    oss.area_code_list        = (uint8 *)f->area_code;
    oss.end_service_code_list = (uint8 *)f->end_service_code;
    
    if (!f->p->polling_and_search_service_code(&polling, &iss, &card_info, &oss)) {
	free(f);
	return NULL;
    }

    f->num_area_code = oss.num_area_codes;
    f->num_service_code = oss.num_service_codes;

    return f;
}

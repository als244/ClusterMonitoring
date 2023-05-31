#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "dcgm_agent.h"
#include "dcgm_fields.h"
#include "dcgm_structs.h"

#define N_FIELDS 28

// pass in memory location of pointer to unallocated array as userdata
int copy_field_values_function(unsigned int gpuId, dcgmFieldValue_v1 * values, int numValues, void * userdata){
	dcgmFieldValue_v1 ** user_field_values_memlocation = (dcgmFieldValue_v1 **) userdata;
	dcgmFieldValue_v1 * user_field_values = malloc(numValues * sizeof(dcgmFieldValue_v1));
	memcpy(user_field_values, values, numValues * sizeof(dcgmFieldValue_v1));
	*user_field_values_memlocation = user_field_values;

	return 0;
}

void save_field_values(dcgmFieldValue_v1 * field_values, int numValues, int iter_num){

	int n_wrote, print_ret;
	char * filepath = NULL;
	FILE * fp;

	print_ret = asprintf(&filepath, "/mnt/storage/research/princeton/monitoring/gpu_test/iterations/%04d.buffer", iter_num);

	fp = fopen(filepath, "wb");
	n_wrote = fwrite(field_values, sizeof(dcgmFieldValue_v1), numValues, fp);
	fclose(fp);
	free(filepath);
}

void cleanup_and_exit(int error_code, dcgmHandle_t * dcgmHandle, dcgmGpuGrp_t * groupId, dcgmFieldGrp_t * fieldGroupId){

	// if cleanup was caused by error
	if (error_code != DCGM_ST_OK){
		printf("ERROR: %s\nFreeing Structs And Exiting...\n", errorString(error_code));
	}

	if (fieldGroupId){
		dcgmFieldGroupDestroy(*dcgmHandle, *fieldGroupId);
	}

	if (groupId){
		dcgmGroupDestroy(*dcgmHandle, *groupId);
	}

	if (dcgmHandle){
		dcgmStopEmbedded(*dcgmHandle);
	}

	dcgmShutdown();

	exit(error_code);

}

int main(int argc, char ** argv){

	dcgmReturn_t dcgm_ret; 

	/* DCGM SETUP */
	dcgm_ret = dcgmInit();

	if (dcgm_ret != DCGM_ST_OK){
		cleanup_and_exit(dcgm_ret, NULL, NULL, NULL);
	}

	dcgmHandle_t dcgmHandle;
	dcgm_ret = dcgmStartEmbedded(DCGM_OPERATION_MODE_AUTO, &dcgmHandle);

	if (dcgm_ret != DCGM_ST_OK){
		cleanup_and_exit(dcgm_ret, &dcgmHandle, NULL, NULL);
	}


	/* READ SYSTEM INFO */

	unsigned int gpuIdList[DCGM_MAX_NUM_DEVICES];
	int gpuCount;

	dcgm_ret = dcgmGetAllSupportedDevices(dcgmHandle, gpuIdList, &gpuCount);

	if (dcgm_ret != DCGM_ST_OK){
		cleanup_and_exit(dcgm_ret, &dcgmHandle, NULL, NULL);
	}	

	// no GPUs in system
	if (gpuCount == 0){
		printf("No GPUs in System, Exiting...\n");
		cleanup_and_exit(dcgm_ret, &dcgmHandle, NULL, NULL);
	}

	// create group with all devices

	// GROUP_DEFAULT creates group with all entities present on system
	char groupName[] = "MyGroup";
	dcgmGpuGrp_t groupId;
	dcgm_ret = dcgmGroupCreate(dcgmHandle, DCGM_GROUP_DEFAULT, groupName, &groupId);

	if (dcgm_ret != DCGM_ST_OK){
		cleanup_and_exit(dcgm_ret, &dcgmHandle, &groupId, NULL);
	}

	// create field group with all the metrics we want to scan

	dcgmFieldGrp_t fieldGroupId;
	char fieldGroupName[] = "MyFieldGroup";

	
	unsigned short fieldIds[N_FIELDS] = {1001, 1002, 1003, 1004, 1013, 1014, 1015, 1016, 1006, 1007, 1008, 203,
								   1005, 204,
								   1009, 1010, 236, 238,
								   1011, 1012,
								   90, 92, 93, 250, 251, 252, 253, 254};


	dcgm_ret = dcgmFieldGroupCreate(dcgmHandle, N_FIELDS, fieldIds, fieldGroupName, &fieldGroupId);


	// watch fields by combining device group and field group

	// update every second
	long long update_freq_micros = 1000;

	// keep for 10 minutes
	double max_keep_seconds = 3600;

	// no limit on samples
	int max_keep_samples = 0;

	dcgm_ret = dcgmWatchFields(dcgmHandle, groupId, fieldGroupId, update_freq_micros, max_keep_seconds, max_keep_samples);

	if (dcgm_ret != DCGM_ST_OK){
		cleanup_and_exit(dcgm_ret, &dcgmHandle, &groupId, &fieldGroupId);
	}

	dcgm_ret = dcgmUpdateAllFields(dcgmHandle, 1);

	if (dcgm_ret != DCGM_ST_OK){
		cleanup_and_exit(dcgm_ret, &dcgmHandle, &groupId, &fieldGroupId);
	}

	// track for an hour
	int max_tracking_seconds = 3600;

	int num_iters = (max_tracking_seconds * 1000) / update_freq_micros;

	dcgmFieldValue_v1 * field_values = NULL;

	for (int i = 0; i < num_iters; i++){

		dcgm_ret = dcgmGetLatestValues(dcgmHandle, groupId, fieldGroupId, &copy_field_values_function, &field_values);

		if (dcgm_ret != DCGM_ST_OK){
			cleanup_and_exit(dcgm_ret, &dcgmHandle, &groupId, &fieldGroupId);
		}

		save_field_values(field_values, N_FIELDS, i);

		free(field_values);

		field_values = NULL;

		usleep(update_freq_micros);
	}
	
	// AT END
	cleanup_and_exit(DCGM_ST_OK, &dcgmHandle, &groupId, &fieldGroupId);

}
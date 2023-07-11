#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>



typedef struct Cpu_stat {
	int cpu_id;
	unsigned long t_user;
	unsigned long t_nice;
    unsigned long t_system;
    unsigned long t_idle;
    unsigned long t_iowait;
    unsigned long t_irq;
    unsigned long t_softirq;
    unsigned long t_steal;
    unsigned long t_guest;
    unsigned long t_guest_nice;
} Cpu_stat;

// typedef struct Counts_stat {
// 	unsigned long len;
// 	unsigned long * counts;
// } Counts_stat;

typedef struct Proc_stat_data{
	unsigned long capture_id;
	unsigned long cur_time;
	unsigned long clk_tck;
	unsigned long n_cpu;
	Cpu_stat ** cpu_stats;
	// Counts_stat * intr;
	// unsigned long ctxt;
	// unsigned long btime;
	// unsigned long processes;
	// unsigned long procs_running;
	// unsigned long procs_blocked;
	// Counts_stat * softirq;
} Proc_stat_data;



Proc_stat_data * process_proc_stat(unsigned long capture_id, unsigned long clk_tck, unsigned long n_cpu){

	FILE * fp = fopen("/proc/stat", "r");

	Proc_stat_data * proc_stat_data = (Proc_stat_data *) malloc(sizeof(Proc_stat_data));

	proc_stat_data -> capture_id = capture_id;
	
	time_t cur_time;
	cur_time = time(NULL);
	proc_stat_data -> cur_time = cur_time;

	proc_stat_data -> clk_tck = clk_tck;
	proc_stat_data -> n_cpu = n_cpu;

	Cpu_stat ** cpu_stats = (Cpu_stat **) malloc(sizeof(Cpu_stat) * (n_cpu + 1));
	for (int i = 0; i < n_cpu + 1; i++){
		cpu_stats[i] = (Cpu_stat *) malloc(sizeof(Cpu_stat));
		cpu_stats[i] -> cpu_id = i - 1;
	}

	// parse all the cpu data

	int cur_cpu_ind = 0;
	char dummy_name[255];
	while (cur_cpu_ind < n_cpu + 1){
		fscanf(fp, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", dummy_name, &(cpu_stats[cur_cpu_ind]->t_user), &(cpu_stats[cur_cpu_ind]->t_nice), 
        &(cpu_stats[cur_cpu_ind]->t_system), &(cpu_stats[cur_cpu_ind]->t_idle), &(cpu_stats[cur_cpu_ind]->t_iowait), &(cpu_stats[cur_cpu_ind]->t_irq),
        &(cpu_stats[cur_cpu_ind]->t_softirq), &(cpu_stats[cur_cpu_ind]->t_steal), &(cpu_stats[cur_cpu_ind]->t_guest), &(cpu_stats[cur_cpu_ind]->t_guest_nice));
        cur_cpu_ind++;
	}

	proc_stat_data -> cpu_stats = cpu_stats;


	// IGNORE FOR NOW
	
	// parse intr line (counts since boot for each of the possible possible interrputs, first col is the total)

	// parse ctxt line (total number of context switches acrsoss all cpus)

	// parse btime line (time at which system booted in seconds since unix epoch)

	// parse processes line (number of processes and threads created, which includes those created by calls to fork() and clone())

	// parse procs_running line (number of processes currently running on CPUs)

	// parse procs_blocked line (number of processes currently blocked, watiing for I/O to complete)

	// parse softirq line (counts of softirqs serviced since boot, first col is the total)


	return proc_stat_data;

}

void dump_proc_stat_data(Proc_stat_data * data){

	// DUMPING DATA
	int n_wrote, print_ret;
	char * filepath = NULL;
	FILE * fp;
	print_ret = asprintf(&filepath, "/mnt/storage/research/princeton/monitoring/cpu_test/iterations/%04lu.buffer", data -> capture_id);
	fp = fopen(filepath, "wb");

	fwrite(&(data -> cur_time), sizeof(unsigned long), 1, fp);
	fwrite(&(data -> clk_tck), sizeof(unsigned long), 1, fp);
	fwrite(&(data -> n_cpu), sizeof(unsigned long), 1, fp);

	Cpu_stat * cur_cpu_stat;

	for (int i = 0; i < (data -> n_cpu) + 1; i++){
		cur_cpu_stat = (data -> cpu_stats)[i];
		fwrite(&(cur_cpu_stat -> cpu_id), sizeof(int), 1, fp);
		fwrite(&(cur_cpu_stat -> t_user), sizeof(unsigned long), 1, fp);
		fwrite(&(cur_cpu_stat -> t_nice), sizeof(unsigned long), 1, fp);
		fwrite(&(cur_cpu_stat -> t_system), sizeof(unsigned long), 1, fp);
		fwrite(&(cur_cpu_stat -> t_idle), sizeof(unsigned long), 1, fp);
		fwrite(&(cur_cpu_stat -> t_iowait), sizeof(unsigned long), 1, fp);
		fwrite(&(cur_cpu_stat -> t_irq), sizeof(unsigned long), 1, fp);
		fwrite(&(cur_cpu_stat -> t_softirq), sizeof(unsigned long), 1, fp);
		fwrite(&(cur_cpu_stat -> t_steal), sizeof(unsigned long), 1, fp);
		fwrite(&(cur_cpu_stat -> t_guest), sizeof(unsigned long), 1, fp);
		fwrite(&(cur_cpu_stat -> t_guest_nice), sizeof(unsigned long), 1, fp);
	}

	fclose(fp);

	return;
}


void destroy_proc_stat_data(Proc_stat_data * data){

	unsigned long n_cpu = data -> n_cpu;

	for (int i = 0; i < n_cpu + 1; i++){
		free((data -> cpu_stats)[i]);
	}
	free(data -> cpu_stats);
	free(data);
}


Proc_stat_data * load_proc_stat_data(unsigned long capture_id){

	// LOADING DATA
	int n_read, print_ret;
	char * filepath = NULL;
	FILE * fp;
	print_ret = asprintf(&filepath, "/mnt/storage/research/princeton/monitoring/cpu_test/iterations/%04lu.buffer", capture_id);
	fp = fopen(filepath, "rb");

	Proc_stat_data * data = (Proc_stat_data *) malloc(sizeof(Proc_stat_data));


	fread(&(data -> cur_time), sizeof(unsigned long), 1, fp);
	fread(&(data -> clk_tck), sizeof(unsigned long), 1, fp);
	fread(&(data -> n_cpu), sizeof(unsigned long), 1, fp);

	unsigned long n_cpu = data -> n_cpu;

	Cpu_stat ** cpu_stats = (Cpu_stat **) malloc(sizeof(Cpu_stat) * (n_cpu + 1));
	Cpu_stat * cur_cpu_stat;
	for (int i = 0; i < n_cpu + 1; i++){
		cur_cpu_stat = (Cpu_stat *) malloc(sizeof(Cpu_stat));
		fread(&(cur_cpu_stat -> cpu_id), sizeof(int), 1, fp);
		fread(&(cur_cpu_stat -> t_user), sizeof(unsigned long), 1, fp);
		fread(&(cur_cpu_stat -> t_nice), sizeof(unsigned long), 1, fp);
		fread(&(cur_cpu_stat -> t_system), sizeof(unsigned long), 1, fp);
		fread(&(cur_cpu_stat -> t_idle), sizeof(unsigned long), 1, fp);
		fread(&(cur_cpu_stat -> t_iowait), sizeof(unsigned long), 1, fp);
		fread(&(cur_cpu_stat -> t_irq), sizeof(unsigned long), 1, fp);
		fread(&(cur_cpu_stat -> t_softirq), sizeof(unsigned long), 1, fp);
		fread(&(cur_cpu_stat -> t_steal), sizeof(unsigned long), 1, fp);
		fread(&(cur_cpu_stat -> t_guest), sizeof(unsigned long), 1, fp);
		fread(&(cur_cpu_stat -> t_guest_nice), sizeof(unsigned long), 1, fp);
		cpu_stats[i] = cur_cpu_stat;
	}

	data -> cpu_stats = cpu_stats;

	fclose(fp);

	return data;
}

void print_proc_stat_data(Proc_stat_data * data){

	printf("Cur Time: %lu, Clk Tck: %lu, N Cpu: %lu\n", data -> cur_time, data -> clk_tck, data -> n_cpu);

	unsigned long n_cpu = data -> n_cpu;

	Cpu_stat * cur_cpu_stat;
	for (int i = 0; i < n_cpu + 1; i++){
		cur_cpu_stat = (data -> cpu_stats)[i];
		printf("Id: %d, User: %lu, Nice: %lu, System: %lu, Idle; %lu, Iowait: %lu, Irq: %lu, Softirq: %lu, Steal %lu, Guest: %lu, Guest Nice: %lu\n", 
				cur_cpu_stat -> cpu_id, cur_cpu_stat -> t_user, cur_cpu_stat -> t_nice, cur_cpu_stat -> t_system, 
				cur_cpu_stat -> t_idle, cur_cpu_stat -> t_iowait, cur_cpu_stat -> t_irq, cur_cpu_stat -> t_softirq, 
				cur_cpu_stat -> t_steal, cur_cpu_stat -> t_guest, cur_cpu_stat -> t_guest_nice);
	}
}



int main(int argc, char * argv[]){

	
	// DUMPING STATS
	// unsigned long capture_id = 1;

	// unsigned long n_cpu = sysconf(_SC_NPROCESSORS_ONLN);
	// unsigned long clock_tick_per_sec = sysconf(_SC_CLK_TCK);

	// Proc_stat_data * data = process_proc_stat(capture_id, clock_tick_per_sec, n_cpu);

	// dump_proc_stat_data(data);

	// destroy_proc_stat_data(data);

	// LOADING STATS

	unsigned long capture_id = 1;

	Proc_stat_data * data = load_proc_stat_data(capture_id);

	print_proc_stat_data(data);

	destroy_proc_stat_data(data);

	return 0;





}


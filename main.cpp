/** @mainpage
@authors {Konik Kothari kkothar3@illinois.edu, Sahil Gupta sjgupta2@illinois.edu}
@version Revision 0.0
@brief This codebase simulates discrete polymer network as
graphs with and without cross-linker bond dynamics. Please 
feel free to add or modify the code as per your usage with the
only condition that this codebase is referenced.
The authors have tried to provide modules for users to build 
their own custom networks and design experiments with them.
The authors do not claim any responsibility for correctness or
accuracy of this code and welcome the community to critique and
improve the code as they wish. This code was written under advisors 
Yuhang Hu and Ahmed Elbanna at University of Illinois at Urbana-Champaign.
@date Thursday April 13, 2017
*/

/**
@file main.cpp
\brief This file includes the code to check whether an MPI implementation 
has been asked by the user and run the experiment accordingly.
*/

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <math.h>
#include <random>
#include <time.h>
#include <vector>
#include <unistd.h>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <stddef.h>
#include <mpi.h>
#include <assert.h>

#include "mpi_network.h"
#include "network.h"
using namespace std;


int main(int argc, char* argv[]) {
	/**< 
	\brief  Takes in 2 command line arguments, <br>
	[1] filename (string) of the .msh file
	generated by GMSH, this file creates the network <br>
	[2] MPI_flag (int) if want to run the code on MPI. 
	Note the syntax for MPI runs.

	Note: mpirun -np <numprocs> <executable> <filename.msh> 1 
	*/
	string path = argv[1];

	#if SACBONDS
	#define DECL_NET sacNetwork test_network(path)
	#else
	#define DECL_NET Network test_network(path)
	#endif
	DECL_NET;

	if(CRACKED){
		// Specific crack
		Crack a;
		a.setter(MAXBOUND_X, MAXBOUND_Y/2.0, MAXBOUND_X/10.0, MAXBOUND_Y/10.0, 0.0, 1.0);

		Cracklist definite_cracks(a);
		test_network.apply_crack(definite_cracks);
		cout<<__LINE__<<endl;

		// Random cracks
		// Cracklist random_cracks(4);
		// test_network.apply_crack(random_cracks);
	}

	//*argv[2] if is working now. Do not change
	if (*argv[2]!='1') {
		cout<<"Running serial version of the code: \n";
		

		// Weight goal: (Because bashing humans over their weight is not enough!)
		float weight_goal = 1.0e6; // weight of similarly sized triangular mesh network
		float weight_multiplier;
		float weight = test_network.get_weight();
		if (weight<weight_goal){
			weight_multiplier = test_network.set_weight(weight_goal);
		}

		bool should_stop = test_network.get_stats();
		
		// add by Dihan to try long links
		test_network.add_long_range_egdes_random(RANDOM_LONG);
		test_network.add_long_range_egdes_y(RANDOM_Y);

		int old_n_edges = test_network.get_current_edges();
		int curr_n_edges = old_n_edges;
		// add by Dihan
		int broke_thres = int(0.9*double(old_n_edges));

		if(should_stop){cout<<"Simulation needs to stop!\n";return 0;}
		float* plate_forces;
		int* remain_chains;

		plate_forces = (float*)malloc(sizeof(float)*DIM*STEPS);
		remain_chains = (int*)malloc(sizeof(int)*STEPS);

		memset(plate_forces, 0.0, STEPS*DIM*sizeof(float));
		memset(remain_chains, 0, STEPS*sizeof(int));

		test_network.plotNetwork(0, true);

		// 
		test_network.dump(0, true);

		// For time is endless but your time...not so much!  
		clock_t t = clock();
		float old_time_per_iter;
		float new_time_per_iter = 100.0f;
		cout<<"\n Will run for "<<STEPS<<":\n";
		
		for(int i = 1; i<STEPS; i++ ){
			// The three lines that do all the work
			test_network.optimize();
			test_network.move_top_plate();

			test_network.get_plate_forces(plate_forces, i);
			curr_n_edges = test_network.get_current_edges();
			// cout << "####" << curr_n_edges << "####" << endl;
			// test_network.get_current_edges(remain_chains, i);
			test_network.get_edge_number(remain_chains, i, curr_n_edges);
			
			// But add more lines, just to show-off.
			// first 100 step
			if (i<=100){
				should_stop = test_network.get_stats();
				// curr_n_edges = test_network.get_current_edges();
				
				if(curr_n_edges<=old_n_edges){
					test_network.plotNetwork(i, false);
					test_network.dump(i);
				}
				if(should_stop){
					break;
				}

				new_time_per_iter = float(clock()-t)/CLOCKS_PER_SEC;
				if(i==0){
					old_time_per_iter = new_time_per_iter;
				}

				if(new_time_per_iter < 0.1*old_time_per_iter){
					cout<<"Seems like very few edges remain! \n";
					break;
				}
				
				// cout<<"Step "<<(i+1)<<" took "<<float(clock()-t)/CLOCKS_PER_SEC<<" s\n";
				t = clock();
				continue;
			}

			if((i+1)%100 == 0){
				should_stop = test_network.get_stats();
				// curr_n_edges = test_network.get_current_edges();
				
				if(curr_n_edges<=old_n_edges){
					test_network.plotNetwork(i, false);
					test_network.dump(i);
				}
				if(should_stop){
					break;
				}


				new_time_per_iter = float(clock()-t)/CLOCKS_PER_SEC;
				if(i==0){
					old_time_per_iter = new_time_per_iter;
				}

				if(new_time_per_iter < 0.1*old_time_per_iter){
					cout<<"Seems like very few edges remain! \n";
					break;
				}
				
				cout<<"Step "<<(i+1)<<" took "<<float(clock()-t)/CLOCKS_PER_SEC<<" s\n";
				t = clock();  // reset clock
				continue;
			}


			// breaking bond steps more details
			if(curr_n_edges < broke_thres && curr_n_edges > 0) {
				should_stop = test_network.get_stats();
				// curr_n_edges = test_network.get_current_edges();
				
				if(curr_n_edges<=old_n_edges){
					test_network.plotNetwork(i, false);
					test_network.dump(i);
				}
				if(should_stop){
					break;
				}

				new_time_per_iter = float(clock()-t)/CLOCKS_PER_SEC;
				if(i==0){
					old_time_per_iter = new_time_per_iter;
				}

				if(new_time_per_iter < 0.1*old_time_per_iter){
					cout<<"Seems like very few edges remain! \n";
					break;
				}
				
				// cout<<"Step "<<(i+1)<<" took "<<float(clock()-t)/CLOCKS_PER_SEC<<" s\n";
				t = clock();
				continue;
			}

		}

		// For names allow unique identification. Well, almost! 
		string sb = SACBONDS ? "true" : "false" ; 
		string fname = FLDR_STRING + std::to_string(L_STD/L_MEAN) + "_" + sb + ".txt";
		string fname2 = FLDR_STRING + std::to_string(L_STD/L_MEAN) + "_" + "remain_chains" + ".txt";
		write_to_file<float>(fname, plate_forces, STEPS, DIM);


		// for(int j = 0; j< STEPS; j++){
		// 	cout << "#####" << remain_chains[j] << endl;
		// }
		write_edge_number<int>(fname2, remain_chains, STEPS);

		free(plate_forces);
		free(remain_chains);
	}
	else {

		// Settle in! This code is murky af
		cout<<"Running MPI version of the code: "<<endl;
		MPI_Init(NULL, NULL);
		
		// Get the number of processes
	  	int world_size;
	  	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	  	// Get the rank of the process
	  	int world_rank;
	  	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	  	MPI_Network * main_network = new MPI_Network(test_network);
		
	  	float* plate_forces = NULL;

  
		
		bool should_stop = main_network->get_stats();
		if(main_network->get_weight()==0){
			cout<<"Problem with MPI constructor! Reading 0 weight! Exiting"<<endl;
			MPI_Finalize();
			return 0;
		}

		if(should_stop || world_size % 2 == 1) {
			//Always take even number of processors
			cout<<"Got stop signal! stop flag: \t"<<should_stop<<\
				"world_size: "<<world_size<<endl;
			MPI_Finalize();
			return 0;
		}
	    
	    // world_rank 0 handles forces and R sync
		if (world_rank == 0) {
			plate_forces = (float*)malloc(sizeof(float)*DIM*STEPS/NSYNC);
			memset(plate_forces, 0, STEPS/NSYNC*DIM*sizeof(float));
		}
		
		// cout<<"world rank:  "<<world_rank<<main_network->n_elems<<endl;
		MPI_Barrier(MPI_COMM_WORLD);
		main_network->init_MPI(world_rank, world_size);

		cout<<__LINE__<<endl;
		MPI_Bcast(main_network->L, main_network->n_elems, MPI_FLOAT, 0, MPI_COMM_WORLD);
		MPI_Bcast(main_network->PBC, main_network->n_elems, MPI_C_BOOL, 0, MPI_COMM_WORLD); 

		size_t r_size = main_network->n_nodes * DIM * world_size;
		float * R_buffer; 
		R_buffer = (float*)malloc(r_size*sizeof(float));//buffer to gather the R from all nodes
		
		float * forces_buffer; 
		forces_buffer = (float*)malloc(r_size*sizeof(float));//buffer to gather the forces from all nodes
		// Force buffer needed to calculate plate_forces
		
		int * chunk_nodes_buffer = new int[main_network->chunk_nodes_len*world_size];
		cout<<"world rank: "<<world_rank<< " chunk len = "<<main_network->chunk_nodes_len<<endl;
				
		MPI_Gather(main_network->chunk_nodes, main_network->chunk_nodes_len, MPI_INT, chunk_nodes_buffer, main_network->chunk_nodes_len, MPI_INT, 0, MPI_COMM_WORLD);

		// Uniqueness of partition check
		if (world_rank == 0) {
			int chunk_sum = 0;
			for (int i = 0; i < main_network->chunk_nodes_len * world_size; i++) {
				if (chunk_nodes_buffer[i] != -1) {
					chunk_sum += chunk_nodes_buffer[i];
				}
			}
			
			int nn = main_network->n_nodes;
			int id;
			for(int d = 0 ; d<main_network->chunk_edges_len; d++){
				id = main_network->chunk_edges[d];
				if(id!=-1){
					if(main_network->edges[2*id] >= nn || main_network->edges[2*id + 1] >= nn){
								cout<<"Node is "<<main_network->edges[2*id]<<" for index "<<id<<endl;
					}	
				}
			}
			if (chunk_sum != (nn*nn - nn)/2 ) {
				cout << chunk_sum << " | "<< (nn*nn - nn)/2<<endl; 
				cout << "Uneven chunk partitioning" << endl;

			}
		}
		
		cout << "World rank proc "<<world_rank << " starting the loop:" << endl;

		int iter = 0; // needed to write forces later

		clock_t t = clock(); 
		for(iter = 0; iter<STEPS; iter++){
			if((iter+1)%100 == 0){
				cout<<"That took "<<(clock()-t)/CLOCKS_PER_SEC<<" s\n";
				t = clock();  // reset clock
				if(world_rank==0){
					cout<<iter+1<<endl; 
					main_network->plotNetwork(iter, false);
					main_network->get_stats();
				}
			}
			main_network->optimize();
			MPI_Barrier(MPI_COMM_WORLD);


			MPI_Gather(main_network->R, main_network->n_nodes * DIM, MPI_FLOAT, R_buffer, main_network->n_nodes * DIM, MPI_FLOAT, 0, MPI_COMM_WORLD);

			if((iter+1)%NSYNC == 0){
				MPI_Gather(main_network->forces, main_network->n_nodes * DIM, MPI_FLOAT, forces_buffer, main_network->n_nodes * DIM, MPI_FLOAT, 0, MPI_COMM_WORLD);
			}

			// syncing R and forces
			if (world_rank == 0) {
				int node_to_sync  = 0;
				for (int i = 0; i < world_size; i += 1) {
					for (int j = i*main_network->chunk_nodes_len; j < (i+1)*main_network->chunk_nodes_len; j++) {
						node_to_sync = chunk_nodes_buffer[j];
						if (node_to_sync == -1) {
							break;
						}
						else{
							main_network->R[DIM * node_to_sync] = R_buffer[main_network->n_nodes * DIM * i + DIM * node_to_sync];
							main_network->R[DIM * node_to_sync + 1] = R_buffer[main_network->n_nodes * DIM * i + DIM * node_to_sync + 1];
							if((iter+1)%NSYNC == 0){
								main_network->forces[DIM * node_to_sync] = forces_buffer[main_network->n_nodes * DIM * i + DIM * node_to_sync];
								main_network->forces[DIM * node_to_sync + 1] = forces_buffer[main_network->n_nodes * DIM * i + DIM * node_to_sync + 1];
							}
						}
					}
					
				}
				if((iter+1)%NSYNC == 0){
					cout << "Synced forces" << endl;
					main_network->get_plate_forces(plate_forces, iter/NSYNC);
				}
				main_network->move_top_plate();
			}

			MPI_Bcast(main_network->R, main_network->n_nodes * DIM, MPI_FLOAT, 0, MPI_COMM_WORLD);
			
			//Barrier required as Bcast is not synchronous and does not block other processes from continuing
			MPI_Barrier(MPI_COMM_WORLD);

		} // the simulation loop ends here


		if (world_rank == 0) {
			string sb = SACBONDS ? "true" : "false" ; 
			string fname = FLDR_STRING + std::to_string(L_STD/L_MEAN) + "_" + sb + ".txt";
			write_to_file<float>(fname, plate_forces, STEPS, DIM);
			free(plate_forces);
			plate_forces = NULL;
		}
		
		//Needed to not have double free, corruption error
		delete[] R_buffer;
		R_buffer = NULL;
		delete[] forces_buffer;
		forces_buffer = NULL;
		delete[] chunk_nodes_buffer;
		chunk_nodes_buffer = NULL;
		delete main_network;
		main_network = NULL;
		cout << "Made it to the end! Exiting now." << endl;
		MPI_Finalize();

	}

	return 0;
}

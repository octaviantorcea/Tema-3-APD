#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#define NR_OF_COORDS 3

#define FIRST_CLUSTER_COORD 0
#define SECOND_CLUSTER_COORD 1
#define THIRD_CLUSTER_COORD 2

#define INPUT_FILE_1 "cluster0.txt"
#define INPUT_FILE_2 "cluster1.txt"
#define INPUT_FILE_3 "cluster2.txt"

int **init_topology(int max_procs) {
    int **topology = (int **) malloc(NR_OF_COORDS * sizeof(int *));

    for (int i = 0; i < NR_OF_COORDS; i++) {
        topology[i] = (int *) malloc(max_procs * sizeof(int));
    }

    return topology;
}

void free_resources(int **topology) {
    for (int i = 0; i < NR_OF_COORDS; i++) {
        free(topology[i]);
    }

    free(topology);
}

// wrapper function that sends an integer and also prints "M(source,dest)" 
void MPI_Send_Int(int value, int source, int dest) {
    printf("M(%d,%d)\n", source, dest);
    MPI_Send(&value, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
}

// wrapper function that sends a vector and also prints "M(source,dest)"
void MPI_Send_Vector(int *vector, int size, int source, int dest) {
    printf("M(%d,%d)\n", source, dest);
    MPI_Send(vector, size, MPI_INT, dest, 0, MPI_COMM_WORLD);
}

// used by every coordinator to read the topology from the input file
void read_input(char *file_name, int rank, int **topology, int *nr_of_workers) {
    // open file
    FILE* topology_file = fopen(file_name, "r");

    if (topology_file == NULL) {
        printf("Can't open input file.\n");
        exit(-1);
    }

    // read nr of workers
    fscanf(topology_file, "%d", &nr_of_workers[rank]);

    for (int i = 0; i < nr_of_workers[rank]; i++) {
        // read workers
        fscanf(topology_file, "%d", &topology[rank][i]);

        // send to workers who is the coordinator
        MPI_Send_Int(rank, rank, topology[rank][i]);
    }

    // close file
    fclose(topology_file);
}

/*
    used by a coordinator to send the data parsed from input file
    to the other coordinators and to its own workers
*/
void broadcast_own_topology(int rank, int **topology, int *nr_of_workers) {
    // send nr of workers to the other coordinators
    for (int other_coord = 0; other_coord < NR_OF_COORDS; other_coord++) {
        if (other_coord != rank) {
            MPI_Send_Int(nr_of_workers[rank], rank, other_coord);
        }
    }

    // send nr of workers to its own workers
    for (int i = 0; i < nr_of_workers[rank]; i++) {
        MPI_Send_Int(nr_of_workers[rank], rank, topology[rank][i]);
    }

    // send worker_vector to the other coordinators
    for (int other_coord = 0; other_coord <  NR_OF_COORDS; other_coord++) {
        if (other_coord != rank) {
            MPI_Send_Vector(topology[rank], nr_of_workers[rank], rank, other_coord);
        }
    }

    // send worker_vector to its own workers
    for (int i = 0; i < nr_of_workers[rank]; i++) {
        MPI_Send_Vector(topology[rank], nr_of_workers[rank], rank, topology[rank][i]);
    }
}

 /*
    used by a coordinator to receive data about topology from
    the other coordinators and to send it to its own workers
  */
void receive_and_scatter_coordinator_topology(int source_coord, int rank, int **topology, int *nr_of_workers) {
    // receive nr of workers
    MPI_Recv(&nr_of_workers[source_coord], 1, MPI_INT, source_coord, 0, MPI_COMM_WORLD, NULL);

    // send nr of workers to its own workers
    for (int i = 0; i < nr_of_workers[rank]; i++) {
        MPI_Send_Int(nr_of_workers[source_coord], rank, topology[rank][i]);
    }

    // receive worker_vector
    MPI_Recv(topology[source_coord], nr_of_workers[source_coord], MPI_INT, source_coord, 0, MPI_COMM_WORLD, NULL);

    // send worker_vector to its own workers
    for (int i = 0; i < nr_of_workers[rank]; i++) {
        MPI_Send_Vector(topology[source_coord], nr_of_workers[source_coord], rank, topology[rank][i]);
    }
}

void print_topology(int **topology, int *nr_of_workers, int rank) {
    printf("%d -> ", rank);

    for (int i = 0; i < NR_OF_COORDS; i++) {
        printf("%d:", i);

        for (int j = 0; j < nr_of_workers[i] - 1; j++) {
            printf("%d,", topology[i][j]);
        }

           printf("%d ", topology[i][nr_of_workers[i] - 1]);
    }

    printf("\n");
}

// used to send parts of the vector that needs to be multiplied by 2
void send_vector_chunk(int *vector, int size, int rank, int dest) {
    // first send the size of the vector chunk
    MPI_Send_Int(size, rank, dest);

    // then send the vector chunk
    MPI_Send_Vector(vector, size, rank, dest);
}

// used to receive parts of the vector that needs to be multiplied by 2
void receive_vector_chunk(int *vector, int *recv_size, int source) {
    // first receive chunk size
    MPI_Recv(recv_size, 1, MPI_INT, source, 0, MPI_COMM_WORLD, NULL);

    // then receive the vector chunk
    MPI_Recv(vector, *recv_size, MPI_INT, source, 0, MPI_COMM_WORLD, NULL);
}

int main(int argc, char* argv[]) {
    int procs, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int **topology = init_topology(procs);
    int nr_of_workers[NR_OF_COORDS];
    int coord = -1;    

    // ---------- ESTABLISH THE TOPOLOGY ----------

    if (rank == FIRST_CLUSTER_COORD) {
        // read input from file
        read_input(INPUT_FILE_1, rank, topology, nr_of_workers);

        // send own topology to the coordinators and its own workers
        broadcast_own_topology(rank, topology, nr_of_workers);

        // receive data related to the topology of the second coordinator and send it to its own workers
        receive_and_scatter_coordinator_topology(SECOND_CLUSTER_COORD, rank, topology, nr_of_workers);

        // receive data related to the topology of the third coordinator and send it to its own workers
        receive_and_scatter_coordinator_topology(THIRD_CLUSTER_COORD, rank, topology, nr_of_workers);
    } else if (rank == SECOND_CLUSTER_COORD) {
        // read input from file
        read_input(INPUT_FILE_2, rank, topology, nr_of_workers);

        // receive data related to the topology of the first coordinator and send it to its own workers
        receive_and_scatter_coordinator_topology(FIRST_CLUSTER_COORD, rank, topology, nr_of_workers);

        // send own topology to the coordinators and its own workers
        broadcast_own_topology(rank, topology, nr_of_workers);

        // receive data related to the topology of the third coordinator and send it to its own workers
        receive_and_scatter_coordinator_topology(THIRD_CLUSTER_COORD, rank, topology, nr_of_workers);
    } else if (rank == THIRD_CLUSTER_COORD) {
        // read input from file
        read_input(INPUT_FILE_3, rank, topology, nr_of_workers);

        // receive data related to the topology of the first coordinator and send it to its own workers
        receive_and_scatter_coordinator_topology(FIRST_CLUSTER_COORD, rank, topology, nr_of_workers);

        // receive data related to the topology of the second coordinator and send it to its own workers
        receive_and_scatter_coordinator_topology(SECOND_CLUSTER_COORD, rank, topology, nr_of_workers);

        // send own topology to the coordinators and its own workers
        broadcast_own_topology(rank, topology, nr_of_workers);
    } else {
        // receive the coordinator
        MPI_Recv(&coord, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, NULL);

        for (int i = 0; i < NR_OF_COORDS; i++) {
            // receive nr of workers for coordinator i
            MPI_Recv(&nr_of_workers[i], 1, MPI_INT, coord, 0, MPI_COMM_WORLD, NULL);

            // receive worker_vector for coordinator i
            MPI_Recv(topology[i], nr_of_workers[i], MPI_INT, coord, 0, MPI_COMM_WORLD, NULL);
        }
    }

    // ---------- PRINT TOPOLOGY ----------

    print_topology(topology, nr_of_workers, rank);

    // ---------- PERFORM CALCULATIONS ----------

    if (rank == FIRST_CLUSTER_COORD) {
        // generate vector
        int vector_size = atoi(argv[1]);
        int vector[vector_size];

        for (int i = 0; i < vector_size; i++) {
            vector[i] = i;
        }

        // compute chunk size for every cluster
        int total_workers = nr_of_workers[FIRST_CLUSTER_COORD] +
                            nr_of_workers[SECOND_CLUSTER_COORD] +
                            nr_of_workers[THIRD_CLUSTER_COORD];
        int part_size = vector_size / total_workers;
        int remaining = vector_size % total_workers;
        int offset = 0;

        for (int coordinator = FIRST_CLUSTER_COORD; coordinator < NR_OF_COORDS; coordinator++) {
            int chunk_size = 0;

            for (int i = 0; i < nr_of_workers[coordinator]; i++) {
                chunk_size += part_size;

                if (remaining > 0) {
                    chunk_size++;
                    remaining--;
                }
            }

            if (coordinator == FIRST_CLUSTER_COORD) {
                int worker_chunk_part = chunk_size / nr_of_workers[FIRST_CLUSTER_COORD];
                int rem_worker = chunk_size % nr_of_workers[FIRST_CLUSTER_COORD];
                int start = 0;

                // send to own workers
                for (int i = 0; i < nr_of_workers[FIRST_CLUSTER_COORD]; i++) {
                    int worker_chunk_size = worker_chunk_part;

                    if (rem_worker > 0) {
                        worker_chunk_size++;
                        rem_worker--;
                    }

                    send_vector_chunk(vector + start, worker_chunk_size, rank, topology[rank][i]);

                    start += worker_chunk_size;
                }
            } else {
                // send vector chunk to the other coordinator
                send_vector_chunk(vector + offset, chunk_size, rank, coordinator);
            }

            offset += chunk_size;
        }

        offset = 0;

        for (int i = 0; i < nr_of_workers[rank]; i++) {
            int recv_chunk_size;

            // receive back the vector chunk from its own workers
            receive_vector_chunk(vector + offset, &recv_chunk_size, topology[rank][i]);
            offset += recv_chunk_size;
        }

        int recv_chunk_size;

        // receive back the vector chunk from the other coordinators
        for (int coordinator = SECOND_CLUSTER_COORD; coordinator < NR_OF_COORDS; coordinator++) {
            receive_vector_chunk(vector + offset, &recv_chunk_size, coordinator);
            offset += recv_chunk_size;
        } 

        // print the final vector
        printf("Rezultat: ");

        for (int i = 0; i < vector_size; i++) {
            printf("%d ", vector[i]);
        }

        printf("\n");
    } else if (rank == SECOND_CLUSTER_COORD || rank == THIRD_CLUSTER_COORD) {
        // receive chunk_size from coordinator 0
        int chunk_size;
        MPI_Recv(&chunk_size, 1, MPI_INT, FIRST_CLUSTER_COORD, 0, MPI_COMM_WORLD, NULL);

        // receive vector chunk from coordinator 0
        int vector[chunk_size];
        MPI_Recv(vector, chunk_size, MPI_INT, FIRST_CLUSTER_COORD, 0, MPI_COMM_WORLD, NULL);

        // compute worker_chunk
        int worker_part = chunk_size / nr_of_workers[rank];
        int remaining = chunk_size % nr_of_workers[rank];
        int offset = 0;

        for (int i = 0; i < nr_of_workers[rank]; i++) {
            int worker_chunk = worker_part;

            if (remaining > 0) {
                worker_chunk++;
                remaining--;
            }

            // send it to its own workers
            send_vector_chunk(vector + offset, worker_chunk, rank, topology[rank][i]);
            offset += worker_chunk;
        }

        offset = 0;

        for (int i = 0; i < nr_of_workers[rank]; i++) {
            int recv_chunk_size;

            // receive back the vector chunk from its own workers
            receive_vector_chunk(vector + offset, &recv_chunk_size, topology[rank][i]);

            offset += recv_chunk_size;
        }

        // send back vector chunk to coordinator 0
        send_vector_chunk(vector, chunk_size, rank, FIRST_CLUSTER_COORD);
    } else {
        // receive worker_chunk_size
        int chunk_size;
        MPI_Recv(&chunk_size, 1, MPI_INT, coord, 0, MPI_COMM_WORLD, NULL);

        // receive vector chunk
        int vec[chunk_size];
        MPI_Recv(vec, chunk_size, MPI_INT, coord, 0, MPI_COMM_WORLD, NULL);

        // multiply by 2
        for (int i = 0; i < chunk_size; i++) {
            vec[i] *= 2;
        }

        // send back vector chunk
        send_vector_chunk(vec, chunk_size, rank, coord);
    }
    
    free_resources(topology);

    MPI_Finalize();
}

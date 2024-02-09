#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

//Function to calculate the average of the vector
double calculateAverage(int *vector, int length){
    double sum = 0.0;
    for (int i = 0; i < length; i++)
    {
        sum += vector[i];
    }
    return sum / length;
}

//Function that calculates the max value inside the vector
int calculateMax(int *vector, int length){
    int max = vector[0];
    for(int i = 0; i < length; i++){
        if(vector[i] > max){
            max = vector[i];
        }
    }
    return max;
}

//Function that calculates the variance of the vector
double calculateVariance(int *vector, int length, double average){
    double squaredDiff = 0.0;
    for (int i = 0; i < length; i++)
    {
        double diff = vector[i] - average;
        squaredDiff += diff * diff;
    }
    return squaredDiff/length;
}

//Function for calculating the new Vector
void calculateVectorD(int *localVector, int *localD, int localMax, int n){
    for(int i = 0; i < n; i++){
        localD[i] = pow(fabs(localVector[i] - localMax), 2); //square of absolute value of element xi minus the max
    }
}


int main(int argc, char *argv[]){

    int rank;
    int size;
    int n; //User given elements for vector
    int *localVector;
    int *localD; //Vector D for each process
    int option; //user given option for the menu

    MPI_Init(&argc, &argv);

    //ID of process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //Total num of processes
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    do{
        //Main process is 0
        if(rank == 0){
            printf("Enter the length of the Vector:\n");
            scanf("%d", &n);

            //Send the vector length to all other processes
            for(int dest = 1;dest < size; dest++){
                MPI_Send(&n, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            }
        }else{
            //Receive the vector length from the main process
            MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //ignoring for now WIP:)
        }

        //Memory allocation for vectors
        localVector = (int *)malloc(n * sizeof(int));
        localD = (int *)malloc(n * sizeof(int));

        //Fills the vector with user elements again on main process
        if(rank == 0){
            printf("Enter %d elements for the vector:\n", n);
            for(int i = 0; i < n; i++){
                scanf("%d", &localVector[i]);
            }

            //Send vector elements to all processes
            for(int dest = 1; dest < size; dest++){
                MPI_Send(localVector, n, MPI_INT, dest, 0, MPI_COMM_WORLD);
            }
        }else{
            MPI_Recv(localVector, n, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //ignoring for now WIP :)
        }

        //Calculate the local average
        double localAverage = calculateAverage(localVector, n);
        int localMax = calculateMax(localVector, n);
        double localVariance = calculateVariance(localVector, n, localAverage);

        calculateVectorD(localVector, localD, localMax, n);

        if (rank != 0)
        {
            MPI_Send(&localAverage, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&localMax, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&localVariance, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
            MPI_Send(localD, n, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }else{
            for (int src = 1; src < size; src++)
            {
                MPI_Recv(&localAverage, 1, MPI_DOUBLE, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&localMax, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&localVariance, 1, MPI_DOUBLE, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(localD, n, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
            }

            printf("Process %d: Global Average: %f\n", rank, localAverage);
            printf("Process %d: Global Max: %d\n", rank, localMax);
            printf("Process %d: Global Variance: %f\n", rank, localVariance);

        }

        if (rank == 0)
        {
            printf("Process %d: New vector D: ",rank);
            for(int i = 0; i < n; i++){
                printf("%d, ", localD[i]);
            }
        }

        //Frees allocated memory
        free(localVector);
        free(localD);
        
        if(rank == 0){
            //User menu
            do{
                printf("\nMenu:\n");
                printf("1. Continue with new numbers.\n");
                printf("2. Exit.\n");
                printf("Enter your option: ");
                scanf("%d", &option);

                if(option != 1 && option != 2){
                    printf("Invalid Option.\n");
                }
            }while(option != 1 && option != 2);

            for(int dest = 1; dest < size; dest++){
                MPI_Send(&option, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            }
        }else{
            MPI_Recv(&option, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

    }while (option == 1);
    
    //Finalize MPI
    MPI_Finalize();

    return 0;
}
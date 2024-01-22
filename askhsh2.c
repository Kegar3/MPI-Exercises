#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "mpi.h"

// Δηλωση struct που θα κραταει τα στοιχεια του ελαχιστου στοιχειου
typedef struct {
    int value;
    int i;
    int j;
} MinValue;

int main(int argc, char** argv) {
    int rank, size, N;
    int **A;
    int isDominant = 1;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Αρχικοποίηση και είσοδος δεδομένων στον αρχικό κόμβο
    if (rank == 0) {
        printf("Enter the size of the matrix: \n");
        scanf("%d", &N);
        
        // Έλεγχος για έγκυρη διάσταση πίνακα Ν πολλαπλάσιο του size
        if(N%size!=0){
            printf("To N prepei na einai pollaplasio tou size\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        // Δέσμευση μνήμης για τον πίνακα A
        A = malloc(N * sizeof(int *));
        //kanoume malloc gia ton NxN pinaka etsi wste h mnhmh na einai sunexhs alliws tha exoume segmentation fault
        A[0] = malloc(N * N * sizeof(int));
        for (int i = 1; i < N; i++) {
            A[i] = A[0] + i * N;
        }

        // Είσοδος των στοιχείων του πίνακα Α
        printf("Enter the elements of the matrix: \n");
        for (int i = 0; i < N; i++) {
            printf("Row %d: ", i + 1);
            for (int j = 0; j < N; j++) {
                scanf("%d", &(A[i][j]));
            }
        }
    }

    // Μοιρασμός της διάστασης N σε όλες τις διεργασίες 
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Δέσμευση μνήμης στις μη αρχικές διεργασίες
    if (rank != 0) {
        A = malloc(N * sizeof(int *));
        A[0] = malloc(N * N * sizeof(int));
        for (int i = 1; i < N; i++) {
            A[i] = A[0] + i * N;
        }
    }

    // Μοιρασμός των στοιχείων του πίνακα A σε όλες τις διεργασίες
    MPI_Bcast(A[0], N*N, MPI_INT, 0, MPI_COMM_WORLD);

    // Εύρεση τοπικού ελάχιστου και ελέγχου για διαγώνια επικράτηση
    int start = rank * N / size;
    int end = (rank + 1) * N / size;

    for (int i = start; i < end; i++) {
        int sum = 0;
        for (int j = 0; j < N; j++) {
            if (i != j) { // Exclude the diagonal element
                sum += abs(A[i][j]);
            }
        }
        // Έλεγχος αν η διαγώνιος είναι κυρίαρχη
        if (abs(A[i][i]) <= sum) {
            isDominant = 0;
        }
    }

    // Συγκέντρωση των αποτελεσμάτων από όλες τις διεργασίες στην αρχική διεργασία
    int global_isDominant;
    MPI_Reduce(&isDominant, &global_isDominant, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);

    // Εκτύπωση αποτελεσμάτων από την αρχική διεργασία
    if (rank == 0) {
        if (global_isDominant == 1) {
            printf("The matrix is strictly diagonally dominant.\n");
        } else {
            printf("The matrix is not strictly diagonally dominant.\n");
        }
        // Εκτύπωση του πίνακα A
        printf("The matrix is:\n");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                printf("%4d", A[i][j]);
            }
            printf("\n");
        }
    }

    // Υπολογισμός της τοπικής μέγιστης απόλυτης τιμής των διαγωνίων στοιχείων
    int local_max = INT_MIN;
    for (int i = start; i < end; i++) {
        if (abs(A[i][i]) > local_max) {
            local_max = abs(A[i][i]);
        }
    }

    // Εύρεση της παγκόσμιας μέγιστης τιμής και διανομή της σε όλες τις διεργασίες
    int global_max;
    MPI_Allreduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    
    // Εκτύπωση της μέγιστης τιμής από την αρχική διεργασία
    if (rank == 0) {
        printf("The maximum absolute value of the diagonal elements is: %d\n", global_max);
    }
    
    int **B;
    B = malloc((N) * sizeof(int *));
    B[0] = malloc(N * N * sizeof(int));
    for (int i = 1; i < N; i++) {
        B[i] = B[0] + i * N;
    }

    // Κάθε διεργασία υπολογίζει τον πίνακα B
    for (int i = start; i < end; i++) {
        for (int j = 0; j < N; j++) {
            if (i == j) {
                B[i-start][j] = global_max;
            } else {
                B[i-start][j] = global_max - abs(A[i][j]);
            }
        }
    }

    // Μεταβλητές για την συλλογή των δεδομένων του πίνακα B στην αρχική διεργασία
    int *recvcounts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        recvcounts[i] = N*N/size;
        displs[i] = i * recvcounts[i];
    }

    // Συλλογή των δεδομένων του πίνακα B στην αρχική διεργασία
    if (rank == 0) {
        int **B_full;
        B_full = malloc(N * sizeof(int *));
        B_full[0] = malloc(N * N * sizeof(int));
        for (int i = 1; i < N; i++) {
            B_full[i] = B_full[0] + i * N;
        }

        MPI_Gatherv(B[0], N*N/size, MPI_INT, B_full[0], recvcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

        printf("The matrix B is:\n");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                printf("%4d", B_full[i][j]);
            }
            printf("\n");
        }

        // Αποδέσμευση μνήμης
        free(B_full[0]);
        free(B_full);
    } else {
        MPI_Gatherv(B[0], N*N/size, MPI_INT, NULL, NULL, NULL, MPI_INT, 0, MPI_COMM_WORLD);
    }

    //!!!douleuei mono gia N akeraio pollaplasio twn arithmwn processes!!!

    // Ευρεση του τοπικου ελαχιστου στοιχειου του πινακα B
    MinValue local_min;
    local_min.value = INT_MAX;
    for (int i = 0; i < N / size; i++) {
        for (int j = 0; j < N; j++) {
            if (B[i][j] < local_min.value) {
                local_min.value = B[i][j];
                local_min.i = i + start;
                local_min.j = j;
            }
        }
    }

    // Συλλογη των τοπικων ελαχιστων στοιχειων στην αρχικη διεργασια
    MinValue* global_data = NULL;
    if (rank == 0) {
        global_data = malloc(size * sizeof(MinValue));
    }
    MPI_Gather(&local_min, sizeof(MinValue), MPI_BYTE, global_data, sizeof(MinValue), MPI_BYTE, 0, MPI_COMM_WORLD);

    // Η αρχικη διεργασια βρισκει το ελαχιστο στοιχειο απο τα τοπικα ελαχιστα στοιχεια
    if (rank == 0) {
        MinValue global_min = global_data[0];
        for (int i = 1; i < size; i++) {
            if (global_data[i].value < global_min.value) {
                global_min = global_data[i];
            }
        }
        // Εκτυπωση του ελαχιστου στοιχειου
        printf("The minimum element of B is %d at position (%d, %d)\n", global_min.value, global_min.i, global_min.j);
        free(global_data);
    }
    //Αποδεσμευση μνημης
    free(A[0]);
    free(A);
    
    free(B[0]);
    free(B);

    free(recvcounts);
    free(displs);
    

    MPI_Finalize();

    return 0;
}
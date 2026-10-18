#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PATIENTS 100
#define MAX_MEDS 200
#define LOG_FILE "adherence_log.txt"

// Represents a Medication Schedule
typedef struct {
    int medID;
    int patientID;
    char medName[50];
    int dosageMg;
    int scheduledHour;   // Hour of day (0 - 23)
    int scheduledMin;    // Minute (0 - 59)
    int taken;           // 0 = Pending/Missed, 1 = Taken
} Prescription;

// Min-Heap node for Priority Queue (Sorted by next scheduled dose time in minutes from midnight)
typedef struct {
    Prescription data;
    int timeInMinutes; // scheduledHour * 60 + scheduledMin
} HeapNode;

typedef struct {
    HeapNode nodes[MAX_MEDS];
    int size;
} PriorityQueue;

// Global Priority Queue
PriorityQueue pq;

// Priority Queue Functions (Min-Heap)
void initPQ(PriorityQueue *q) {
    q->size = 0;
}

void swapNodes(HeapNode *a, HeapNode *b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

void insertPQ(PriorityQueue *q, Prescription p) {
    if (q->size >= MAX_MEDS) {
        printf("Priority Queue is full!\n");
        return;
    }

    int timeMin = p.scheduledHour * 60 + p.scheduledMin;
    q->nodes[q->size].data = p;
    q->nodes[q->size].timeInMinutes = timeMin;

    int curr = q->size;
    q->size++;

    // Heapify Up
    while (curr > 0 && q->nodes[curr].timeInMinutes < q->nodes[(curr - 1) / 2].timeInMinutes) {
        swapNodes(&q->nodes[curr], &q->nodes[(curr - 1) / 2]);
        curr = (curr - 1) / 2;
    }
}

// Log adherence events to file
void logAdherenceEvent(const char *patientName, const char *medName, const char *status, int hour, int min) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return;

    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    fprintf(fp, "[%04d-%02d-%02d %02d:%02d] Patient: %s | Med: %s | Scheduled: %02d:%02d | Status: %s\n",
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_hour, timeinfo->tm_min,
            patientName, medName, hour, min, status);

    fclose(fp);
}

// Add a new Prescription
void addPrescription(void) {
    Prescription p;
    printf("\n--- Add New Prescription Schedule ---\n");
    printf("Enter Patient ID: ");
    scanf("%d", &p.patientID);
    printf("Enter Medication ID: ");
    scanf("%d", &p.medID);
    getchar(); // Clear newline

    printf("Enter Medication Name: ");
    fgets(p.medName, sizeof(p.medName), stdin);
    p.medName[strcspn(p.medName, "\n")] = '\0';

    printf("Enter Dosage (mg): ");
    scanf("%d", &p.dosageMg);

    printf("Enter Scheduled Time Hour (0-23): ");
    scanf("%d", &p.scheduledHour);

    printf("Enter Scheduled Time Minute (0-59): ");
    scanf("%d", &p.scheduledMin);

    p.taken = 0;

    insertPQ(&pq, p);
    printf("Prescription added successfully and scheduled!\n");
}

// Display all pending schedules ordered by time
void displayUpcomingSchedule(void) {
    if (pq.size == 0) {
        printf("\nNo active prescription schedules.\n");
        return;
    }

    printf("\n%-10s %-10s %-20s %-10s %-12s %-8s\n", 
           "Med ID", "Patient ID", "Medication Name", "Dosage", "Scheduled", "Status");
    printf("-------------------------------------------------------------------------\n");

    for (int i = 0; i < pq.size; i++) {
        Prescription p = pq.nodes[i].data;
        printf("%-10d %-10d %-20s %-10dmg %02d:%02d        %-8s\n",
               p.medID, p.patientID, p.medName, p.dosageMg,
               p.scheduledHour, p.scheduledMin,
               p.taken ? "TAKEN" : "PENDING");
    }
}

// Simulate patient interaction (taking medication or missing dose)
void simulateAdherence(void) {
    if (pq.size == 0) {
        printf("\nNo pending prescriptions to simulate.\n");
        return;
    }

    int currentHour, currentMin;
    printf("\n--- Simulated Time Check ---\n");
    printf("Enter Current Time Hour (0-23): ");
    scanf("%d", &currentHour);
    printf("Enter Current Time Minute (0-59): ");
    scanf("%d", &currentMin);

    int currentTimeInMin = currentHour * 60 + currentMin;

    printf("\n--- Checking Scheduled Doses against Current Time (%02d:%02d) ---\n", currentHour, currentMin);

    for (int i = 0; i < pq.size; i++) {
        HeapNode *node = &pq.nodes[i];
        if (node->data.taken == 1) continue;

        int schedMin = node->timeInMinutes;
        
        // If current time is past scheduled time by 30 mins -> Trigger ALERT
        if (currentTimeInMin > schedMin + 30) {
            printf("[ALERT] MISSED DOSE! Patient ID %d missed %s (%dmg) scheduled for %02d:%02d!\n",
                   node->data.patientID, node->data.medName, node->data.dosageMg,
                   node->data.scheduledHour, node->data.scheduledMin);
            
            logAdherenceEvent("Patient_Record", node->data.medName, "MISSED_ALERT_SENT",
                               node->data.scheduledHour, node->data.scheduledMin);
        }
        // If current time is within 30 min window -> Prompt to take
        else if (currentTimeInMin >= schedMin - 15 && currentTimeInMin <= schedMin + 30) {
            char choice;
            printf("\n[REMINDER] Time to take %s (%dmg) for Patient ID %d (Scheduled: %02d:%02d).\n",
                   node->data.medName, node->data.dosageMg, node->data.patientID,
                   node->data.scheduledHour, node->data.scheduledMin);
            printf("Mark as TAKEN now? (y/n): ");
            scanf(" %c", &choice);

            if (choice == 'y' || choice == 'Y') {
                node->data.taken = 1;
                printf("Dose marked as TAKEN.\n");
                logAdherenceEvent("Patient_Record", node->data.medName, "TAKEN_ON_TIME",
                                   node->data.scheduledHour, node->data.scheduledMin);
            } else {
                printf("Dose marked as PENDING.\n");
            }
        }
    }
}

// Display stored audit log
void viewAdherenceLogs(void) {
    FILE *fp = fopen(LOG_FILE, "r");
    if (!fp) {
        printf("\nNo logs found yet.\n");
        return;
    }

    printf("\n--- Adherence Audit Logs (`%s`) ---\n", LOG_FILE);
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    fclose(fp);
}

int main(void) {
    initPQ(&pq);
    int choice;

    while (1) {
        printf("\n======================================================\n");
        printf("    SMART PRESCRIPTION & ADHERENCE TRACKER (DSA)      \n");
        printf("======================================================\n");
        printf("1. Add Prescription Schedule\n");
        printf("2. View Scheduled Medication Queue (Min-Heap)\n");
        printf("3. Simulate Current Time & Check Adherence\n");
        printf("4. View Adherence Audit File Logs\n");
        printf("5. Exit\n");
        printf("Enter choice (1-5): ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input! Exiting...\n");
            break;
        }

        switch (choice) {
            case 1: addPrescription(); break;
            case 2: displayUpcomingSchedule(); break;
            case 3: simulateAdherence(); break;
            case 4: viewAdherenceLogs(); break;
            case 5:
                printf("Exiting Tracker system.\n");
                exit(0);
            default:
                printf("Invalid choice! Select between 1 and 5.\n");
        }
    }
    return 0;
} 
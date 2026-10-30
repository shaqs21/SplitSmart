#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 20
#define NAME_LEN 30
#define DESC_LEN 40

typedef struct Person{
    char name[NAME_LEN];
    long balance;
}Person;

typedef struct Expense{
    char desc[DESC_LEN];
    long amount;
    int payer;
    int split[MAX];
    struct Expense *next;
}Expense;

typedef struct Heap{
    int index[MAX];
    int size;
}Heap;

Person people[MAX];
int n=0;
long debt[MAX][MAX];
Expense *top=NULL;
char tripname[60]="My Trip";
Heap creditors;
Heap debtors;


static void readLine(const char *prompt, char *buf, int size){
    printf("%s",prompt);
    if(fgets(buf,size,stdin)==NULL){
        printf("\nInput closed. Exiting.\n");
        exit(0);
    }
    if(strchr(buf,'\n')==NULL){
        int ch;
        while((ch=getchar())!='\n' && ch!=EOF);
    }
    buf[strcspn(buf,"\n")]='\0';
}

static long readLong(const char *prompt){
    char b[64];
    readLine(prompt,b,sizeof(b));
    return atol(b);
}

static int countExpenses(void){
    int count=0;
    Expense *temp=top;
    while(temp!=NULL){
        count++;
        temp=temp->next;
    }
    return count;
}

static void addMember(void){
    if(n>=MAX){
        printf("Sorry, the group is full (maximum %d members).\n",MAX);
        return;
    }
    char name[NAME_LEN];
    readLine("Enter the member's name: ",name,sizeof(name));
    if (name[0]=='\0'){
        printf("The name can't be empty. Please try again.\n");
        return;
    }
    strncpy(people[n].name,name,NAME_LEN-1);
    people[n].name[NAME_LEN-1]='\0';
    people[n].balance=0;
    n++;
    printf("Added %s to the group!\n",name);
}

static void showMembers(void){
    for(int i=0;i<n;i++){
        printf(" %d. %s",i+1,people[i].name);
        printf("\n");
    }
}


static void addExpense(void){
    if(n<2){
        printf("You need at least 2 members before adding an expense.\n");
        return;
    }
    char desc[DESC_LEN];
    readLine("What was it for? (e.g. Hotel, Dinner): ",desc, sizeof desc);
    long amount= readLong("How much was it (Rs.)? ");
    if(amount<=0){
        printf("The amount must be more than Rs. 0.\n");
        return;
    }
    printf("Who paid for it?\n");
    showMembers();
    int payer= readLong("Enter their number: ")-1;
    if (payer<0 || payer>=n){
        printf("That number isn't in the list.\n");
        return;
    }
    int split[MAX]={0};
    char ans[10];
    readLine("Is it shared by everyone? (y/n): ",ans,sizeof(ans));
    if(ans[0]=='y' || ans[0]=='Y'){
        for(int i=0;i<n;i++){
            split[i]=1;
        }
    }
    else{
        for(int i=0;i<n;i++){
            char question[80];
            snprintf(question,sizeof(question),"Does %s share this one? (y/n): ",people[i].name);
            readLine(question,ans,sizeof(ans));
            if(ans[0]=='y' || ans[0]=='Y'){
                split[i]=1;
            }
        }
    }
    int k=0;
    for(int i=0;i<n;i++){
        k+=split[i];
    }
    if(k==0){
        printf("Nobody is sharing this expense, so nothing was added.\n");
        return;
    }
    long base=amount/k;
    long rem=amount%k;
    for(int i=0;i<n;i++){
        if(split[i]){
            long share=base;
            if(rem>0){
                share++;
                rem--;
            }
            people[i].balance-=share;
            if(i!=payer){
                debt[i][payer]+=share;
            }
        }
    }
    people[payer].balance+=amount;
    Expense *e=calloc(1,sizeof(Expense));
    strncpy(e->desc,desc,DESC_LEN-1);
    e->amount=amount;
    e->payer=payer;
    for(int i=0;i<n;i++){
        e->split[i]=split[i];
    }
    e->next=top;
    top=e;
    printf("Done! %s paid Rs. %ld for %s (split between %d).\n",people[payer].name,amount,desc,k);
}


static void showBalances(void){
    printf("\n--- Who is ahead and who is behind ---\n");
    if(n==0){
        printf("No members yet. Add someone with option 1.\n");
        return;
    }
    for(int i=0;i<n;i++){
        long b=people[i].balance;
        if(b>0){
            printf("%-12s gets back Rs. %ld\n",people[i].name,b);
        }
        else if(b<0){
            printf("%-12s owes Rs. %ld\n",people[i].name,-b);
        }
        else{
            printf("%-12s is all settled\n",people[i].name);
        }
    }
}


static void showDebts(void){
    int found=0;
    printf("\n--- Who owes whom (before settling) ---\n");
    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
            if(debt[i][j]>0){
                printf("%s owes %s Rs. %ld\n",people[i].name,people[j].name,debt[i][j]);
                found++;
            }
        }
    }
    if(found==0){
        printf("Nobody owes anything right now.\n");
    }
}

static void undoLastExpense(void){
    if(top==NULL){
        printf("There are no expenses to undo.\n");
        return;
    }
    Expense *e= top;
    top=e->next;
    people[e->payer].balance-=e->amount;
    int k=0;
    for(int i=0;i<n;i++){
        k+=e->split[i];
    }
    long base=e->amount/k;
    long rem=e->amount%k;
    for(int i=0;i<n;i++){
        if(e->split[i]){
            long share=base;
            if(rem>0){
                share++;
                rem--;
            }
            people[i].balance+=share;
            if(i!=e->payer)
            debt[i][e->payer]-=share;
        }
    }
    printf("Removed the last expense (%s, Rs. %ld).\n",e->desc,e->amount);
    free(e);
}

static void initHeaps(void){
    creditors.size=0;
    debtors.size=0;
}

static void heapInsert(Heap *h,int personIndex){
    int i=h->size;
    h->index[i]=personIndex;
    h->size++;
    while(i>0){
        int parent=(i-1)/2;
        if(people[h->index[parent]].balance>=people[h->index[i]].balance){
            break;
        }
        int temp=h->index[parent];
        h->index[parent]=h->index[i];
        h->index[i]=temp;
        i=parent;
    }
}


static void debtorHeapInsert(Heap *h, int personIndex){
    int i=h->size;
    h->index[i]=personIndex;
    h->size++;
    while(i>0){
        int parent=(i-1)/2;
        if(people[h->index[parent]].balance<=people[h->index[i]].balance){
            break;
        }
        int temp=h->index[parent];
        h->index[parent]=h->index[i];
        h->index[i]=temp;
        i=parent;
    }
}

static int heapExtract(Heap *h){
    int result=h->index[0];
    h->size--;
    h->index[0]=h->index[h->size];
    int i=0;
    while(1){
        int left=2*i+1;
        int right=2*i+2;
        int largest=i;
        if(left<h->size && people[h->index[left]].balance>people[h->index[largest]].balance){
            largest=left;
        }
        if(right<h->size && people[h->index[right]].balance>people[h->index[largest]].balance){
            largest=right;
        }
        if(largest==i)
        break;
        int temp=h->index[i];
        h->index[i]=h->index[largest];
        h->index[largest]=temp;
        i=largest;
    }
    return result;
}

static int debtorHeapExtract(Heap *h){
    int result=h->index[0];
    h->size--;
    h->index[0]=h->index[h->size];
    int i=0;
    while(1){
        int left=2*i+1;
        int right=2*i+2;
        int smallest=i;
        if(left<h->size && people[h->index[left]].balance<people[h->index[smallest]].balance){
            smallest=left;
        }
        if(right<h->size && people[h->index[right]].balance<people[h->index[smallest]].balance){
            smallest=right;
        }
        if(smallest==i){
            break;
        }
        int temp=h->index[i];
        h->index[i]=h->index[smallest];
        h->index[smallest]=temp;
        i=smallest;
    }
    return result;
}

static void buildHeaps(void){
    creditors.size=0;
    debtors.size=0;
    for(int i=0;i<n;i++){
        if(people[i].balance>0){
            heapInsert(&creditors,i);
        }
        else if(people[i].balance<0){
            debtorHeapInsert(&debtors,i);
        }
    }
}

static void settle(void){
    long backup[MAX];
    for(int i=0;i<n;i++){
        backup[i]=people[i].balance;
    }
    buildHeaps();
    if(creditors.size==0 && debtors.size==0){
        printf("\nEveryone is already settled. Nobody needs to pay anything!\n");
        return;
    }
    int payments=0;
    printf("\n--- Who should pay whom ---\n");
    while(creditors.size>0 && debtors.size>0){
        int creditor=heapExtract(&creditors);
        int debtor=debtorHeapExtract(&debtors);
        long owe= -people[debtor].balance;
        long receive=people[creditor].balance;
        long amount;
        if(owe<receive){
            amount=owe;
        }
        else
        amount=receive;
        printf("%s should pay %s Rs. %ld\n",people[debtor].name,people[creditor].name,amount);
        payments++;
        people[debtor].balance+=amount;
        people[creditor].balance-=amount;
        if(people[debtor].balance<0){
            debtorHeapInsert(&debtors,debtor);
        }
        if(people[creditor].balance>0){
            heapInsert(&creditors,creditor);
        }
    }
    for(int i=0;i<n;i++){
        people[i].balance=backup[i];
    }
    printf("\nAll done! Everyone is even after just %d payment(s).\n",payments);
}

static void saveData(void){
    FILE *fp=fopen("splitsmart.txt","w");
    if(fp==NULL){
        printf("Sorry, the trip could not be saved.\n");
        return;
    }
    fprintf(fp,"%s\n",tripname);
    fprintf(fp,"%d\n",n);
    for(int i=0;i<n;i++){
        fprintf(fp,"%s\n",people[i].name);
        fprintf(fp,"%ld\n",people[i].balance);
    }
    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
            fprintf(fp,"%ld ",debt[i][j]);
        }
        fprintf(fp,"\n");
    }
    int count=0;
    Expense *temp=top;
    while(temp!=NULL){
        count++;
        temp=temp->next;
    }
    fprintf(fp,"%d\n",count);
    temp=top;
    while(temp!=NULL){
        fprintf(fp,"%s\n",temp->desc);
        fprintf(fp,"%ld\n",temp->amount);
        fprintf(fp,"%d\n",temp->payer);
        for(int i=0;i<n;i++){
            fprintf(fp,"%d ",temp->split[i]);
        }
        fprintf(fp,"\n");
        temp=temp->next;
    }
    fclose(fp);
    printf("Trip saved!\n");
}

static void loadData(void){
    FILE *fp= fopen("splitsmart.txt","r");
    if(fp==NULL){
        printf("No saved trip found.\n");
        return;
    }
    while(top!=NULL){
        Expense *temp=top;
        top=top->next;
        free(temp);
    }
    memset(people,0,sizeof(people));
    memset(debt,0,sizeof(debt));
    fgets(tripname,60,fp);
    tripname[strcspn(tripname,"\n")]='\0';
    fscanf(fp,"%d\n",&n);
    if(n>MAX){
        n=MAX;
    }
    for(int i=0;i<n;i++){
        fgets(people[i].name,NAME_LEN,fp);
        people[i].name[strcspn(people[i].name,"\n")]='\0';
        fscanf(fp,"%ld\n",&people[i].balance);
    }
    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
            fscanf(fp,"%ld",&debt[i][j]);
        }
    }
    int count;
    fscanf(fp,"%d\n",&count);
    Expense *tail=NULL;
    for(int x=0;x<count;x++){
        Expense *e=calloc(1,sizeof(Expense));
        fgets(e->desc,DESC_LEN,fp);
        e->desc[strcspn(e->desc,"\n")]='\0';
        fscanf(fp,"%ld\n",&e->amount);
        fscanf(fp,"%d\n",&e->payer);
        for(int i=0;i<n;i++){
            fscanf(fp,"%d",&e->split[i]);
        }
        fscanf(fp,"\n");
        e->next=NULL;
        if(tail==NULL){
            top=e;
        }
        else{
            tail->next=e;
        }
        tail=e;
    }
    fclose(fp);
    printf("Trip loaded! %d members, %d expenses.\n",n,countExpenses());
}



static void menuLine(const char *text){
    printf("| %-40s |\n",text);
}

static void printMenu(void){
    char line[80];
    printf("\n+------------------------------------------+\n");
    menuLine("        SplitSmart - Group Expenses");
    printf("+------------------------------------------+\n");
    snprintf(line,sizeof(line),"Trip: %.33s",tripname);
    menuLine(line);
    snprintf(line,sizeof(line),"Members: %d      Expenses: %d",n,countExpenses());
    menuLine(line);
    printf("+------------------------------------------+\n");
    menuLine("1.  Add a member");
    menuLine("2.  See all members");
    menuLine("3.  Add an expense");
    menuLine("4.  See who is ahead / behind");
    menuLine("5.  See who owes whom");
    menuLine("6.  Undo the last expense");
    menuLine("7.  Settle up (fewest payments)");
    menuLine("8.  Save trip");
    menuLine("9.  Load saved trip");
    menuLine("10. Exit");
    printf("+------------------------------------------+\n");
}

int main(){
    initHeaps();
    loadData();
    int choice;

    while(1){
        printMenu();

        choice = readLong("Enter your choice (1-10): ");

        switch(choice){

            case 1:
                addMember();
                break;

            case 2:
                if(n==0){
                    printf("\nNo members yet. Add someone with option 1.\n");
                }
                else{
                    printf("\n--- Group members (%d) ---\n",n);
                    showMembers();
                }
                break;

            case 3:
                addExpense();
                break;

            case 4:
                showBalances();
                break;

            case 5:
                showDebts();
                break;

            case 6:
                undoLastExpense();
                break;

            case 7:
                settle();
                break;

            case 8:
                saveData();
                break;

            case 9:
                loadData();
                break;

            case 10:
                saveData();
                printf("SplitSmart closed!\n");
                break;

            default:
                printf("Invalid option. Please enter a number from 1 to 10.\n");
        }

        if(choice == 10){
            break;
        }
    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define OFFSETBITS 5
#define LENGTHBITS (8-OFFSETBITS)

#define OFFSETMASK ((1 << (OFFSETBITS)) - 1)
#define LENGTHMASK ((1 << (LENGTHBITS)) - 1)

#define GETOFFSET(x) (x >> LENGTHBITS)
#define GETLENGTH(x) (x & LENGTHMASK)
#define OFFSETLENGTH(x,y) (x << LENGTHBITS | y)

#define MAX_TREE_HT 100
struct token
{
    uint8_t offset_len;
    char c;
};

/*
* iki string'in ilk kaç karakteri özdeş?
* maksimum limit sayısı kadar kontrol yapar.
*/
int onek_eslesme_uzunlugu(char *s1, char *s2, int limit)
{
    int len = 0;

    while (*s1++ == *s2++ && len < limit)
        len++;
    return len;
}

/*
* memcpy fonksiyonu ile benzer. Byte düzeyinde
* kopyalama yapma garantisi olduðu için, bu
* versiyonu kullanıyoruz.
*/
void lz77kopyalama(char *s1, char *s2, int size)
{
    while (size--)
        *s1++ = *s2++;
}

/*
* token array'ini, karakter array'ine dönüştürür.
*/
char *decode(struct token *tokens, int numTokens, int *pcbDecoded)
{
    // hafızada ayırdığımız kapasite
    int cap = 1 << 3;

    // kullanılan byte sayısı
    *pcbDecoded = 0;

    // hafızada yer ayır
    char *decoded = malloc(cap);

    int i;
    for (i = 0; i < numTokens; i++)
    {
        // token'in içinden offset, length ve char
        // değerlerini oku
        int offset = GETOFFSET(tokens[i].offset_len);
        int len = GETLENGTH(tokens[i].offset_len);
        char c = tokens[i].c;

        // gerekirse kapasite artır.
        if (*pcbDecoded + len + 1 > cap)
        {
            cap = cap << 1;
            decoded = realloc(decoded, cap);
        }

        // eðer length 0 değilse, gösterilen karakteleri
        // kopyala
        if (len > 0)
        {
            lz77kopyalama(&decoded[*pcbDecoded], &decoded[*pcbDecoded - offset], len);
        }

        // kopyalanan karakter kadar ileri sar
        *pcbDecoded += len;

        // tokenin içindeki karateri ekle.
        decoded[*pcbDecoded] = c;

        // 1 adım daha ileri sar.
        *pcbDecoded = *pcbDecoded + 1;
    }

    return decoded;
}

/*
* LZ77 ile sıkıştırılacak bir metin alır.
* token array'i döndürür.
* Eğer numTokens NULL değilse, token sayısını
* numTokens ile gösterilen yere kaydeder.
* char *text => sıkıştırılacak metin
* int limit  => Kaç karakter sıkıştıracağımız.
* int *numTokens => Kullanılan token sayısının yerini gösterir pointer
*/

struct token *encode(char *text, int limit, int *numTokens)
{
    // cap (kapasite) hafızada kaç tokenlik yer ayırdığımız.
    int cap = 1 << 3;

    // kaç token oluşturduğumuz.
    int _numTokens = 0;

    // oluşturulacak token
    struct token t;

    // lookahead ve search buffer'larının başlangıç pozisyonları.
    char *lookahead, *search;
// tokenler için yer ayır.
    struct token *encoded = malloc(cap * sizeof(struct token));

    // token oluşturma döngüsü
    for (lookahead = text; lookahead < text + limit; lookahead++)
    {
        // search buffer'ı lookahead buffer'ın 31 (OFFSETMASK) karakter
        // gerisine koyuyoruz.
        search = lookahead - OFFSETMASK;

        // search buffer'ın metnin dışına çıkmasına engel ol.
        if (search < text)
            search = text;

        // search bufferda bulunan en uzun eşleşmenin
        // boyutu
        int max_len = 0;

        // search bufferda bulunan en uzun eşleşmenin
        // pozisyonu
        char *max_match = lookahead;

        // search buffer içinde arama yap.
        for (; search < lookahead; search++)
        {
            int len = onek_eslesme_uzunlugu(search, lookahead, LENGTHMASK);

            if (len > max_len)
            {
                max_len = len;
                max_match = search;
            }
        }

        /*
        * ! ÖNEMLİ !
        * Eğer eşleşmenin içine metnin son karakteri de dahil olmuşsa,
        * tokenin içine bir karakter koyabilmek için, eşleşmeyi kısaltmamız
        * gerekiyor.
        */
        if (lookahead + max_len >= text + limit)
        {
            max_len = text + limit - lookahead - 1;
        }


        // bulunan eşleşmeye göre token oluştur.
        t.offset_len = OFFSETLENGTH(lookahead - max_match, max_len);
        lookahead += max_len;
        t.c = *lookahead;

        // gerekirse, hafızada yer aç
        if (_numTokens + 1 > cap)
        {
            cap = cap << 1;
            encoded = realloc(encoded, cap * sizeof(struct token));
        }

        // oluşturulan tokeni, array'e kaydet.
        encoded[_numTokens++] = t;
    }

    if (numTokens)
        *numTokens = _numTokens;

    return encoded;
}

// bir dosyanın tamamını tek seferde
// okur. Büyük dosyaları okumak için uygun
// değildir.
char *file_read(FILE *f, int *size)
{
    char *content;
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    content = malloc(*size);
    fseek(f, 0, SEEK_SET);
    fread(content, 1, *size, f);
    return content;
}


// Huffman agac dugumu
struct MinHeapNode
{
    char data;
    unsigned freq;
    struct MinHeapNode *left, *right;
};

struct MinHeap
{
    unsigned size;
    unsigned capacity;
    struct MinHeapNode** array;
};

struct MinHeapNode* newNode(char data, unsigned freq)
{
    struct MinHeapNode* temp
        = (struct MinHeapNode*)malloc
          (sizeof(struct MinHeapNode));

    temp->left = temp->right = NULL;
    temp->data = data;
    temp->freq = freq;

    return temp;
}

struct MinHeap* createMinHeap(unsigned capacity)

{

    struct MinHeap* minHeap
        = (struct MinHeap*)malloc(sizeof(struct MinHeap));

    // mevcut boyut 0
    minHeap->size = 0;

    minHeap->capacity = capacity;

    minHeap->array
        = (struct MinHeapNode**)malloc(minHeap->
                                       capacity * sizeof(struct MinHeapNode*));
    return minHeap;
}

void swapMinHeapNode(struct MinHeapNode** a,
                     struct MinHeapNode** b)

{

    struct MinHeapNode* t = *a;
    *a = *b;
    *b = t;
}

void minHeapify(struct MinHeap* minHeap, int idx)

{

    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < minHeap->size && minHeap->array[left]->
            freq < minHeap->array[smallest]->freq)
        smallest = left;

    if (right < minHeap->size && minHeap->array[right]->
            freq < minHeap->array[smallest]->freq)
        smallest = right;

    if (smallest != idx)
    {
        swapMinHeapNode(&minHeap->array[smallest],
                        &minHeap->array[idx]);
        minHeapify(minHeap, smallest);
    }
}

int isSizeOne(struct MinHeap* minHeap)
{

    return (minHeap->size == 1);
}

struct MinHeapNode* extractMin(struct MinHeap* minHeap)

{

    struct MinHeapNode* temp = minHeap->array[0];
    minHeap->array[0]
        = minHeap->array[minHeap->size - 1];

    --minHeap->size;
    minHeapify(minHeap, 0);

    return temp;
}

void insertMinHeap(struct MinHeap* minHeap,
                   struct MinHeapNode* minHeapNode)

{

    ++minHeap->size;
    int i = minHeap->size - 1;

    while (i && minHeapNode->freq < minHeap->array[(i - 1) / 2]->freq)
    {

        minHeap->array[i] = minHeap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }

    minHeap->array[i] = minHeapNode;
}

void buildMinHeap(struct MinHeap* minHeap)

{

    int n = minHeap->size - 1;
    int i;

    for (i = (n - 1) / 2; i >= 0; --i)
        minHeapify(minHeap, i);
}

void printArr(int arr[], int n)
{
    int i;
    for (i = 0; i < n; ++i)
        printf("%d", arr[i]);

    printf("\n");
}

int isLeaf(struct MinHeapNode* root)

{

    return !(root->left) && !(root->right);
}

struct MinHeap* createAndBuildMinHeap(char data[], int freq[], int size)

{

    struct MinHeap* minHeap = createMinHeap(size);

    for (int i = 0; i < size; ++i)
        minHeap->array[i] = newNode(data[i], freq[i]);

    minHeap->size = size;
    buildMinHeap(minHeap);

    return minHeap;
}

struct MinHeapNode* buildHuffmanTree(char data[], int freq[], int size)

{
    struct MinHeapNode *left, *right, *top;

    // 1. Adım:Minimum kapasite yığını oluşturun boyuta eşit.

    struct MinHeap* minHeap = createAndBuildMinHeap(data, freq, size);

    while (!isSizeOne(minHeap))
    {

        // 2. Adım: İki minimum değeri çıkarın
        left = extractMin(minHeap);
        right = extractMin(minHeap);

        /* 3. Adım: Sıklığı iki düğüm frekansının toplamına eşit
        yeni bir iç düğüm oluşturun. Çıkarılan iki düğümü
        bu yeni düğümün sol ve sağ çocukları olarak yapın.
        Bu düğümü min öbeğine ekleyin '$' iç düğümler için özel bir değerdir, kullanılmamış
        */

        top = newNode('$', left->freq + right->freq);

        top->left = left;
        top->right = right;

        insertMinHeap(minHeap, top);
    }

    // 4. Adım: Kalan düğüm kök düğümdür ve ağaç tamamlanmıştır
    return extractMin(minHeap);
}

void printCodes(struct MinHeapNode* root, int arr[], int top)

{

    if (root->left)
    {

        arr[top] = 0;
        printCodes(root->left, arr, top + 1);
    }

    if (root->right)
    {

        arr[top] = 1;
        printCodes(root->right, arr, top + 1);
    }

    if (isLeaf(root))
    {

        printf("%c: ", root->data);
        printArr(arr, top);
    }
}

void HuffmanCodes(char data[], int freq[], int size)

{
    struct MinHeapNode* root
        = buildHuffmanTree(data, freq, size);

    //Yukarıda oluşturulan Huffman ağacını kullanarak Huffman kodlarını yazdırın
    int arr[MAX_TREE_HT], top = 0;

    printCodes(root, arr, top);
}

int main(void)
{
    FILE *dosya;
    int metin_boyutu, token_sayisi, decode_boyutu;
    char dosyaismi[100];

    char *decoded_metin = "";
    struct token *encoded_metin;
    int i=0, k=0;

    printf("Dosyanin Adini Giriniz: ");
    scanf("%s",dosyaismi);

    FILE *m;
    int aa=0,cc=1,last;//Burada ifadelerimizi tanımladık.
    char ch;//Karakter olarak ch ifadesini tanımladık.

    m=fopen(dosyaismi,"r");//Burada fopen içerisine dosyamızın tam adını yazıyoruz.Ben mmsrn.txt olarak oluşturmuştum siz değiştirebilirsiniz.
    while(!feof(m)) //Siz "for" döngüsüne de alabilirsiniz keyfinize göre size kalmış.
    {
        ch=getc(m);
        aa++;//aa ifadesini 1 arttırıyoruz.

        if(ch=='\n') //Eğer ch ifadesi boşluğa denk gelirse satır sayısını 1 arttırıyoruz.
        {
            cc++;
        }
    }

    fclose(m);//Yazı yazılan metin belgesini kapatıyoruz.

    metin_boyutu = aa-cc;
    char *kaynak_metin[metin_boyutu];
    FILE *fpointer;
    fpointer = fopen(dosyaismi,"r");

    if( (fpointer = fopen(dosyaismi,"r")) != NULL)
    {
        while(!feof(fpointer))
        {
            // printf("girdi");
            fscanf(fpointer,"%s",&kaynak_metin);
            //  printf("%s",kaynak_metin);

        }
    }
    else
    {
        printf("dosya yok");
    }

    fclose(fpointer);

    encoded_metin = encode(kaynak_metin, metin_boyutu, &token_sayisi);

    if (dosya = fopen("encoded.txt", "wb"))
    {
        fwrite(encoded_metin, sizeof(struct token), token_sayisi, dosya);
        fclose(dosya);
    }

    decoded_metin = decode(encoded_metin, token_sayisi, &decode_boyutu);

    if (dosya = fopen("decoded.txt", "wb"))
    {
        fwrite(decoded_metin, 1, decode_boyutu, dosya);
        fclose(dosya);
    }

    printf("LZ77:\nOrjinal Boyut: %d, Encode Boyutu: %d, Decode Boyutu: %d\n", metin_boyutu, token_sayisi * sizeof(struct token), decode_boyutu);
// LZ77 algoritması burada bitiyor.

    FILE *dosya2;
    dosya2 = fopen(dosyaismi,"r");
    int frekans[26];
    int j=0;
    char karakter;
    int metinboyutu=0;
    int sayac =0;
    while ((karakter = fgetc(dosya2)) != EOF)
    {
        metinboyutu++;
        //printf("karakter: %c (%d)\n", d, d);

    }
    char metin[metinboyutu];
    int x =0 ;
    if(( dosya2 = fopen(dosyaismi,"r"))!= NULL)
    {
        metin[0] = fgetc(dosya2);
        while(metin[x] != EOF)
        {
            // printf("%c",metin[x]);
            x++;
            metin[x] = fgetc(dosya2);
        }
    }
    fclose(dosya2);

    for(int i=0; i<26; i++)
        frekans[i]=0;
    for(int i=0; i<strlen(metin); i++)
    {
        if(metin[i]=='a'||metin[i]=='A')
            frekans[0]++;
        else if(metin[i]=='b'||metin[i]=='B')
            frekans[1]++;
        else if(metin[i]=='c'||metin[i]=='C')
            frekans[2]++;
        else if(metin[i]=='d'||metin[i]=='D')
            frekans[3]++;
        else if(metin[i]=='e'||metin[i]=='E')
            frekans[4]++;
        else if(metin[i]=='f'||metin[i]=='F')
            frekans[5]++;
        else if(metin[i]=='g'||metin[i]=='G')
            frekans[6]++;
        else if(metin[i]=='h'||metin[i]=='H')
            frekans[7]++;
        else if(metin[i]=='i'||metin[i]=='I')
            frekans[8]++;
        else if(metin[i]=='j'||metin[i]=='J')
            frekans[9]++;
        else if(metin[i]=='k'||metin[i]=='K')
            frekans[10]++;
        else if(metin[i]=='l'||metin[i]=='L')
            frekans[11]++;
        else if(metin[i]=='m'||metin[i]=='M')
            frekans[12]++;
        else if(metin[i]=='n'||metin[i]=='N')
            frekans[13]++;
        else if(metin[i]=='o'||metin[i]=='O')
            frekans[14]++;
        else if(metin[i]=='p'||metin[i]=='P')
            frekans[15]++;
        else if(metin[i]=='q'||metin[i]=='Q')
            frekans[16]++;
        else if(metin[i]=='r'||metin[i]=='R')
            frekans[17]++;
        else if(metin[i]=='s'||metin[i]=='S')
            frekans[18]++;
        else if(metin[i]=='t'||metin[i]=='T')
            frekans[19]++;
        else if(metin[i]=='u'||metin[i]=='U')
            frekans[20]++;
        else if(metin[i]=='v'||metin[i]=='V')
            frekans[21]++;
        else if(metin[i]=='w'||metin[i]=='W')
            frekans[22]++;
        else if(metin[i]=='x'||metin[i]=='X')
            frekans[23]++;
        else if(metin[i]=='y'||metin[i]=='Y')
            frekans[24]++;
        else if(metin[i]=='z'||metin[i]=='Z')
            frekans[25]++;
    }

    for(int i=0 ; i<26; i++)
    {
        //printf("\n%d\n",frekans[i]);
    }
    int sayac3 =0;
    char arr[] = { 'a', 'b', 'c', 'd', 'e', 'f','g', 'h', 'i', 'j', 'k', 'l','m', 'n', 'o', 'p', 'q', 'r','s', 't', 'u', 'v', 'w', 'x','y', 'z'};
    for(int i =0 ; i<26 ; i++)
    {
        if(frekans[i]!=0)
        {
            sayac3++;

        }

    }
    int frekansyeni[sayac3];
    int sayacfrekans = 0;
    char arryeni[sayac3];

    for(int i =0 ; i<26 ; i++)
    {
        if(frekans[i]!=0)
        {
            arryeni[sayacfrekans] = arr[i];
            frekansyeni[sayacfrekans]==frekans[i];
            sayacfrekans++;


        }

    }

    int size = sizeof(arryeni) / sizeof(arryeni[0]);

    printf("\nHuffman Agaci:\n");

    HuffmanCodes(arryeni, frekansyeni, size);

    return 0;
}

#include <ctype.h>
#include <stdio.h>
#include "zutil.h"
#include "derivor.h"
#include "moreassert.h"
#include <assert.h>

const static size_t MAXWORDSIZE=64;

void
read_vocabulary_file(FILE*const vocabulary_file, Pvoid_t*const pVOC) {
  PWord_t ptr;
  char token[MAXWORDSIZE];
  // int lines = 0;

  while (fgets((char*)token, MAXWORDSIZE, vocabulary_file) != (char *)NULL) {

    size_t ln = strlen(token) - 1;
    if (token[ln] == '\n')
      token[ln] = '\0';

    JSLI(ptr, *pVOC, (unsigned char*)token);
    // lines++;
  }
  // fprintf(stderr, "Finished reading vocabulary: %u\n", lines); fflush(stderr);

}

bool
inV(const czstr c, const Pvoid_t*const pVOC) {
        PWord_t ptr;

        JSLG(ptr, pVOC, c.buf);
        if (ptr == NULL) {
                return false;
        } else {
                return true;
        }
}

void
print_jt(Pvoid_t*const T) {
  PWord_t ptr;
  zbyte index[MAXWORDSIZE+1];
  zstr z;

  index[0] = '\0';
  JSLF(ptr, *T, index);

  while (ptr != NULL) {
      z = repr(cs_as_cz((char*)index));
      printf("%s\n", z.buf);
      free(z.buf);
      JSLG(ptr, *T, index);
      //printf("%f\n", *(float*)ptr);
      JSLN(ptr, *T, index);
  }
}

bool
_print_if_in(const czstr c, const Pvoid_t*const pVOC, int lineno, char const* funcname, char type, char first, char second) {
        bool suc = false;
        if (inV(c, pVOC)) {
                fwrite(c.buf, 1, c.len, stdout);
                fwrite(" ", 1, 1, stdout);
                printf("%c %c %c, ", type, first, second);
                suc = true;
        }
#ifndef NDEBUG
        if (suc) {
                printf(" generated by %s, line %d\n", funcname, lineno);
        }
#endif
        return suc;
}

#define print_if_in(c, pVOC, type, first, second) _print_if_in(c, pVOC, __LINE__, __func__, type, first, second)

void
derive(const czstr c, const Pvoid_t*const pVOC) {
        /*printf("about to czcheck3\n");*/
        assert (cz_check(c));
        /*printf("done to czcheck3\n");*/

	/* This was to constrain derivations to come only from words in the vocabulary.
	   Now leaving the decision to the client code.

	   if (!inV(c, pVOC)) {
	       printf("\n");
               fflush(stdout);
               return;
	       } */

        int i;
        for (i=0; i<c.len; i++) {
            if (!isalpha(c.buf[i])) {
                printf("\n");
                fflush(stdout);
                return;
            }
        }

        zbyte palimpsest[DERIVORMAXWORDSIZE+2]; /* +1 for insertion, +1 for null-terminator */
        zstr pz = { 0, palimpsest };
        char j;

        /*printf("deriving "); fwrite(c.buf, 1, c.len, stdout); printf(" len %lu...\n", c.len);*/

        runtime_assert(c.len <= DERIVORMAXWORDSIZE, "Argument c to derive() is required to be <= DERIVORMAXWORDSIZE.");
        runtime_assert(c.len >= 1, "Argument c to derive() is required to have length >= 1.");

        /* deletions */
        /* i will be the index of the deleted char */
        if (c.len > 1) {
                pz.len = c.len-1;
                pz.buf[pz.len] = '\0';
                memcpy(pz.buf, c.buf+1, c.len-1);
                print_if_in(cz(pz), pVOC, 'd', '@', c.buf[0]);
                pz.buf[0] = c.buf[0];
                for (i=1; i<c.len; i++) {
                        memcpy(pz.buf+i, c.buf+i+1, c.len-i-1);
                        print_if_in(cz(pz), pVOC, 'd', pz.buf[i-1], c.buf[i]);
                        pz.buf[i] = c.buf[i];
                }
        }

        /* substitutions */
        /* i will be the index of the substituted char. */
        memcpy(pz.buf, c.buf, c.len);
        pz.len = c.len;
        pz.buf[pz.len] = '\0';
        for (i=0; i<c.len; i++) {
                for (j='a'; j<='z'; j++) {
                        if (j != c.buf[i]) {
                                pz.buf[i] = j;
                                print_if_in(cz(pz), pVOC, 's', c.buf[i], j);
                                pz.buf[i] = c.buf[i];
                        }
                }
        }

        /* transpositions */
        /* i will be the index of the first (leftmost) transposed char. */
        memcpy(pz.buf, c.buf, c.len);
        pz.len = c.len;
        pz.buf[pz.len] = '\0';
        for (i=0; i<c.len-1; i++) {
                pz.buf[i] = c.buf[i+1];
                pz.buf[i+1] = c.buf[i];
                if (c.buf[i] != pz.buf[i] || c.buf[i+1] != pz.buf[i+1]) {
                        print_if_in(cz(pz), pVOC, 't', pz.buf[i+1], c.buf[i+1]);
                }
                /* put it back */
                pz.buf[i] = c.buf[i];
        }

        /* insertions */
        /* i will be the index of the new char. */
        pz.len = c.len+1;
        pz.buf[pz.len] = '\0';
        memcpy(pz.buf+1, c.buf, c.len);
        for (j='a'; j<='z'; j++) {
                pz.buf[0] = j;
                print_if_in(cz(pz), pVOC, 'i', '@', j);
        }
        pz.buf[0] = c.buf[0];
        for (i=1; i<c.len; i++) {
                for (j='a'; j<='z'; j++) {
                        pz.buf[i] = j;
                        print_if_in(cz(pz), pVOC, 'i', pz.buf[i-1], j);
                }
                pz.buf[i] = c.buf[i];
        }
        for (j='a'; j<='z'; j++) {
                pz.buf[i] = j;
                print_if_in(cz(pz), pVOC, 'i', pz.buf[i-1], j);
        }

        printf("\n"); fflush(stdout);
}


int
main(int argc, char** argv) {
        if (argc < 2) {
                printf("usage: derivor vocabulary_file\n");
                return 1;
        }

        FILE* vocabulary_file = fopen(argv[1], "r");
        Pvoid_t VOC = (Pvoid_t) NULL; 

        read_vocabulary_file(vocabulary_file, &VOC);

        zbyte inputbuf[MAXWORDSIZE + 4];
        zbyte* p;
        PWord_t ptr;

        size_t i;
        do {
                fgets((char*)inputbuf, MAXWORDSIZE + 4, stdin);
                i = strlen((char*)inputbuf);
                if (inputbuf[i-1] == '\n') {
                        inputbuf[--i] = '\0';
                } else {
                        inputbuf[i] = '\0';
                }
                p = inputbuf;

                if (*p == 'v') {
                        p+=2; // command and space
                        JSLG(ptr, VOC, p);
                        if (ptr == NULL) {
                                printf("0\n");
                                fflush(stdout);
                        }
                        else {
                                printf("1\n");
                                fflush(stdout);
                        }
                }

                else if (*p == 'd') {
                        p+=2; // command and space
                        const czstr cz = {i-2, p};
                        /*printf("czcheck1\n");*/
                        assert (cz_check(cz));
                        /*printf("czcheck1 end\n");*/
                        derive(cz, VOC);
                }

                else if (*p == 'p') {
		  print_jt(&VOC);
                }



        } while (
            (i > 0) &&
            (feof(stdin) == 0) &&
            (ferror(stdin) == 0)
            );

        Word_t temp;
        JSLFA(temp, VOC);

        fflush(stdout);
        return 0;
}

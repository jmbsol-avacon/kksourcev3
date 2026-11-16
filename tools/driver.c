#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "db_defs.h"

PRNCODES prnCodes;

VOID WritePrinterDriver(const CHAR *, PRNCODES *);

int main(void)
{
    strcpy(prnCodes.model, "Epson Compatable");
    strcpy(prnCodes.cond, "\x1b\x0f");
    strcpy(prnCodes.reset, "\x12");
    strcpy(prnCodes.land, "");
    WritePrinterDriver("EPSON.DRV", &prnCodes);
    WritePrinterDriver("DEFAULT.DRV", &prnCodes);
    return 0;
}

VOID WritePrinterDriver(const CHAR *prnDriver, PRNCODES *codes)
{
    INT fh;

    Assert(prnDriver != NULL);

    if ((fh = open(prnDriver, O_CREAT|O_BINARY|O_RDWR|O_TRUNC, S_IREAD|S_IWRITE)) != -1) {
        write(fh, codes, sizeof(PRNCODES));
        close(fh);
        printf("\nCreated Printer Driver %s", prnDriver);
    }
}

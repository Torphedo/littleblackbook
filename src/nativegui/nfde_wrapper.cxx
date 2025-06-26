#include <vector>
#include <string>
#include <nfd.h>

#include <common/int.h>

nfdresult_t NFD_OpenDialogMultipleAutoFree(std::vector<std::string>& output, const nfdu8filteritem_t* filterList,
                                     nfdfiltersize_t filterCount, const nfdu8char_t* defaultPath) {
    const nfdpathset_t* pathset;
    auto result = NFD_OpenDialogMultipleU8(&pathset, filterList, filterCount, defaultPath);
    if (result != NFD_OKAY) {
        return result;
    }

    nfdpathsetsize_t numPaths = 0;
    if (NFD_PathSet_GetCount(pathset, &numPaths) != NFD_OKAY) {
        NFD_PathSet_Free(pathset);
        return NFD_ERROR;
    }

    output.reserve(numPaths);
    for (u32 i = 0; i < numPaths; i++) {
        char* path = nullptr;
        if (NFD_PathSet_GetPathN(pathset, i, &path) != NFD_OKAY) {
            continue; // Invalid path or something
        }

        output.push_back(path);
        NFD_PathSet_FreePathN(path);
    }

    // Free NFD's stuff
    NFD_PathSet_Free(pathset);
    return NFD_OKAY;
}

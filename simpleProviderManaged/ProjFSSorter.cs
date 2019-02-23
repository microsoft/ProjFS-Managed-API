// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

using System.Collections.Generic;
using Microsoft.Windows.ProjFS;

namespace SimpleProviderManaged
{
    /// <summary>
    /// Implements IComparer using <see cref="Microsoft.Windows.ProjFS.Utils.FileNameCompare(string, string)"/>.
    /// </summary>
    internal class ProjFSSorter : Comparer<string>
    {
        public override int Compare(string x, string y) => Utils.FileNameCompare(x, y);
    }
}

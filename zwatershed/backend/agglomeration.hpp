#pragma once

#include "types.hpp"

#include <zi/disjoint_sets/disjoint_sets.hpp>
#include <map>
#include <vector>
#include <set>

template<typename V, typename F, typename FN>
inline void
merge_segments_with_function(
        V& seg,
        const region_graph_ptr<typename V::element, F> rg_ptr,
        counts_t<std::size_t>& counts,
        const FN& func,
        size_t low,
        bool recreate_rg)
{
    typedef typename V::element ID;

    zi::disjoint_sets<ID> sets(counts.size());

    region_graph<ID,F>& rg  = *rg_ptr;

    for ( auto& edge: rg )
    {
        std::size_t size = func(edge.weight);

        if ( size == 0 )
        {
            break;
        }

        ID s1 = sets.find_set(edge.id1);
        ID s2 = sets.find_set(edge.id2);

        // std::cout << s1 << " " << s2 << " " << size << "\n";

        if ( s1 != s2 && s1 && s2 )
        {
            if ( (counts[s1] < size) || (counts[s2] < size) )
            {
                counts[s1] += counts[s2];
                counts[s2]  = 0;
                ID s = sets.join(s1,s2);
                std::swap(counts[s], counts[s1]);
            }
        }
    }

    std::vector<ID> remaps(counts.size());

    ID next_id = 1;

    for ( ID id = 0; id < counts.size(); ++id )
    {
        ID s = sets.find_set(id);
        if ( s && (remaps[s] == 0) && (counts[s] >= low) )
        {
            remaps[s] = next_id;
            counts[next_id] = counts[s];
            ++next_id;
        }
    }

    counts.resize(next_id);

    std::ptrdiff_t xdim = seg.shape()[0];
    std::ptrdiff_t ydim = seg.shape()[1];
    std::ptrdiff_t zdim = seg.shape()[2];

    std::ptrdiff_t total = xdim * ydim * zdim;

    ID* seg_raw = seg.data();

    for ( std::ptrdiff_t idx = 0; idx < total; ++idx )
    {
        seg_raw[idx] = remaps[sets.find_set(seg_raw[idx])];
    }

    std::cout << "\tDone with remapping, total: " << (next_id-1) << std::endl;

    region_graph<ID,F> new_rg;

    std::vector<std::set<ID>> in_rg(next_id);

    for ( auto& edge: rg )
    {
        ID s1 = remaps[sets.find_set(edge.id1)];
        ID s2 = remaps[sets.find_set(edge.id2)];

        if ( s1 != s2 && s1 && s2 )
        {
            auto mm = std::minmax(s1,s2);
            if ( in_rg[mm.first].count(mm.second) == 0 )
            {
                new_rg.push_back(region_graph_edge_t<F,ID>(edge.weight, mm.first, mm.second));
                in_rg[mm.first].insert(mm.second);
            }
        }
    }

    if(recreate_rg)
        rg.swap(new_rg);

    std::cout << "\tDone with updating the region graph, size: "
              << rg.size() << std::endl;
}
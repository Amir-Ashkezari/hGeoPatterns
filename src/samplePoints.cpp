#include "RixPattern.h"
#include "RixPredefinedStrings.hpp"

#include "hGeoStructsRIS.h"

#include <GU/GU_Detail.h>
#include <GEO/GEO_PointTree.h>

#include <map>
#include <iostream>

class samplePoints: public RixPattern
{
public:
	enum paramId
	{
		// outputs
		ARRAY_DATA_IDS(IdxA)
		ARRAY_DATA_IDS(DistA)

		// inputs
		k_filename,
		k_numPoints,
		k_coordsys,

		// end of list
		k_numParams
	};

	struct Data
	{
		RtInt numPoints;
		RtUString coordsys;
		GU_Detail *gdp;
		GEO_PointTreeGAOffset *tree;
	};

	int Init(RixContext &ctx, RtUString const pluginpath) override;

	RixSCParamInfo const *GetParamTable() override;

	void Synchronize(
		RixContext&,
		RixSCSyncMsg,
		RixParameterList const*) override
		{
		}

	void CreateInstanceData(RixContext&,
							RtUString const,
							RixParameterList const*,
							InstanceData*) override;

	void Finalize(RixContext &ctx) override;

	int ComputeOutputParams(RixShadingContext const *sCtx,
							int *nOutputs,
							RixPattern::OutputSpec **outputs,
							RtPointer instanceData,
							RixSCParamInfo const *instanceTable) override;

	bool Bake2dOutput(  RixBakeContext const*,
						Bake2dSpec&,
						RtPointer) override
	{
		return false;
	}

	bool Bake3dOutput(  RixBakeContext const*,
						Bake3dSpec&,
						RtPointer) override
	{
		return false;
	}

private:
	std::unordered_map<RtUString, std::pair<GU_Detail*,GEO_PointTreeGAOffset*>> m_trees;
};


int
samplePoints::Init(RixContext &ctx, RtUString const pluginpath)
{
	PIXAR_ARGUSED(ctx);
	PIXAR_ARGUSED(pluginpath);

	return 0;
}


void
samplePoints::Finalize(RixContext &ctx)
{
	for (auto item: m_trees)
	{
		delete item.second.first; // gdp
		delete item.second.second; // tree
	}
	PIXAR_ARGUSED(ctx);
}


RixSCParamInfo const*
samplePoints::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		// outputs
		ARRAY_DATA_OUT("IdxA")
		ARRAY_DATA_OUT("DistA")

		// inputs
		RixSCParamInfo(RtUString("filename"), k_RixSCString),
		RixSCParamInfo(RtUString("numPoints"), k_RixSCInteger),
		RixSCParamInfo(RtUString("coordsys"), k_RixSCString),

		// end of table
		RixSCParamInfo()
	};
	return &s_ptable[0];
}


void samplePoints::CreateInstanceData(RixContext& ctx,
										RtUString const handle,
										RixParameterList const* params,
										InstanceData* instanceData)
{
	PIXAR_ARGUSED(ctx);
	PIXAR_ARGUSED(handle);

	instanceData->datalen = sizeof(Data);
	instanceData->data = malloc(instanceData->datalen);
	instanceData->freefunc = free;
	Data *data = static_cast<Data*>(instanceData->data);

	RtUString filename = US_NULL;
	params->EvalParam(k_filename, -1, &filename);

	data->numPoints = 1;
	params->EvalParam(k_numPoints, -1, &data->numPoints);

	data->coordsys = Rix::k_object;
	params->EvalParam(k_coordsys, -1, &data->coordsys);
	if (data->coordsys.Empty())
		data->coordsys = Rix::k_object;

	// std::cout << data->coordsys.CStr() << std::endl;

	data->gdp = nullptr;
	data->tree = nullptr;

	if (!filename.Empty())
	{
		auto it = m_trees.find(filename);

		if (it == m_trees.end())
		{
			GU_Detail * gdp = new GU_Detail;
			if (gdp->load(filename.CStr()).success())
			{
				GEO_PointTreeGAOffset *tree = new GEO_PointTreeGAOffset;
				tree->build(gdp);
				m_trees[filename] = std::pair<GU_Detail*,GEO_PointTreeGAOffset*>(gdp, tree);
				data->gdp = gdp;
				data->tree = tree;
				// std::cout << "Loaded "<< gdp->getNumPoints() << " points: " << filename.CStr() << " " << tree->getMemoryUsage(true) << std::endl;
			}
		}
		else
		{
			data->gdp = it->second.first;
			data->tree = it->second.second;
		}
	}

	return;
}


int
samplePoints::ComputeOutputParams(RixShadingContext const *sCtx,
									int *nOutputs,
									OutputSpec **outputs,
									RtPointer instanceData,
									RixSCParamInfo const *instanceTable)
{

	PIXAR_ARGUSED(instanceTable);

	Data const* data = static_cast<Data const*>(instanceData);

	if (!data->tree)
		return 1;

	RixSCType type;
	RixSCConnectionInfo cinfo;
	RixSCParamInfo const* paramTable = GetParamTable();
	int numOutputs = -1;
	while (paramTable[++numOutputs].access == k_RixSCOutput) {}

	RixShadingContext::Allocator pool(sCtx);
	OutputSpec *out = *outputs = pool.AllocForPattern<OutputSpec>(numOutputs);
	*nOutputs = numOutputs;

	// looping through the different output ids
	bool connected = true; // possible optimization
	int variable_num = 0;
	for (int i = 0; i < numOutputs; ++i)
	{
		out[i].paramId = i;
		out[i].detail = k_RixSCInvalidDetail;
		out[i].value = NULL;

		type = paramTable[i].type; // we know this

		sCtx->GetParamInfo(i, &type, &cinfo);

		if (type == k_RixSCStructEnd)
			continue;

		else if (type == k_RixSCStructBegin)
		{
			variable_num = 0;
		}

		else if( connected && type == k_RixSCInteger )
		{
			out[i].detail = k_RixSCUniform;
			out[i].value = pool.AllocForPattern<RtInt>(1);
		}

		else if( connected && type == k_RixSCFloat3 )
		{
			// Allocate only needed number of variables
			if (variable_num < data->numPoints){
				out[i].detail = k_RixSCVarying;
				out[i].value = pool.AllocForPattern<RtFloat3>(sCtx->numPts);
			}
			variable_num++;
		}
	}

	// as optimisation we pass pointer to loaded geometry to next reader
	out[k_IdxA_meshID].value = (RtInt*)data->gdp;
	out[k_DistA_meshID].value = (RtInt*)data->gdp;

	RtInt* IdxA_num = (RtInt*) out[k_IdxA_num].value;
	RtInt* DistA_num = (RtInt*) out[k_DistA_num].value;

	RtFloat3 *resultI[8];
	RtFloat3 *resultD[8];
	for (int n=0; n<data->numPoints; n++)
	{
		resultI[n] = (RtFloat3*) out[k_IdxA_v0+n].value;
		resultD[n] = (RtFloat3*) out[k_DistA_v0+n].value;
	}

	RtFloat3 const *P;
	RtFloat3 *Pw = pool.AllocForPattern<RtPoint3>(sCtx->numPts);

	// __Pref and Po are not defined for volumes
	if (sCtx->scTraits.volume != NULL)
	{
		sCtx->GetBuiltinVar(RixShadingContext::k_P, &P);
	}
	else
	{
		RixSCDetail pDetail = sCtx->GetPrimVar(RtUString("__Pref"), RtFloat3(0.0f), &P);
		if (pDetail == k_RixSCInvalidDetail)
			sCtx->GetBuiltinVar(RixShadingContext::k_Po, &P);
	}

	memcpy(Pw, P, sizeof(RtFloat3) * sCtx->numPts);

	sCtx->Transform(RixShadingContext::k_AsPoints, Rix::k_current, data->coordsys, Pw, NULL);
	UT_Vector3 pos;
	GEO_PointTree::IdxArrayType plist;
	UT_FloatArray distances;
	int found = -1;

	for (int i = 0; i < sCtx->numPts; ++i)
	{
		pos.x() = Pw[i].x;
		pos.y() = Pw[i].y;
		pos.z() = Pw[i].z;
		found = data->tree->findNearestGroupIdx(pos, FLT_MAX, data->numPoints, plist, distances);

		//number of sampled points
		for (int n=0; n<data->numPoints; n++)
		{
			if(n<found)
			{
				resultD[n][i] = RtFloat3(sqrt(distances[n]));
				resultI[n][i] = RtFloat3(plist[n]);
			}
			else
			{
				resultD[n][i] = RtFloat3(-1);
				resultI[n][i] = RtFloat3(-1);
			}
		}

	}

	IdxA_num[0] = found;
	DistA_num[0] = found;

	return 0;
}


RIX_PATTERNCREATE
{
	PIXAR_ARGUSED(hint);

	return new samplePoints();
}

RIX_PATTERNDESTROY
{
	delete static_cast<samplePoints*>(pattern);
}

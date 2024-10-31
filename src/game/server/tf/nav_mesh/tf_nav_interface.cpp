/* TF Nav Mesh
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_nav_interface.h"
#include "tf_nav_mesh.h"


LINK_ENTITY_TO_CLASS(tf_point_nav_interface, CPointNavInterface);


BEGIN_DATADESC(CPointNavInterface)
	
	DEFINE_INPUTFUNC(FIELD_VOID, "RecomputeBlockers", RecomputeBlockers),
	
END_DATADESC()


void CPointNavInterface::RecomputeBlockers(inputdata_t& inputdata)
{
	if (TheTFNavMesh != nullptr) {
		TheTFNavMesh->RecomputeBlockers();
	}
}

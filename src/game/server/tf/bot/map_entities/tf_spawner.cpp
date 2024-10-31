/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_spawner.h"


//#pragma message("TODO: put tf_spawner in the FGD!")


// TODO


/* from tf.fgd:

@PointClass base(Targetname) = tf_spawner : "An entity that spawns templatized entities."
[
	count(integer) : "Count" : 1 : "Total number of entities to spawn over the lifetime of this spawner."
	maxActive(integer) : "Max Active" : 1 : "Maximum number of simultaneous active entities created by this spawner."
	interval(float) : "Interval" : 0 : "Time (in seconds) between spawns"
	template(target_destination) : "Template to spawn entities from"

	// Inputs
	input Enable(void) : "Begin spawning entities"
	input Disable(void) : "Stop spawning entities"
	input Reset(void) : "Reset spawner to initial state"

	// Outputs
	output OnSpawned(void) : "Sent when an entity has spawned into the environment"
	output OnExpended(void) : "Sent when the spawner has reached its allowed total of entities spawned"
	output OnKilled(void) : "Sent when en entity spawned by this spawner has died/been destroyed"
]

*/

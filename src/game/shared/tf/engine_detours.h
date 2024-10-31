#include <memy/memytools.h>
#include <memy/detourhook.hpp>

class CEngineDetours : public CAutoGameSystem
{
public:
                        CEngineDetours();

    void                PostInit() override;

};

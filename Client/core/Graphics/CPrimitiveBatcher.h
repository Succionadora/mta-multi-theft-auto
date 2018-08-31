/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        CPrimitiveBatcher.h
 *  PURPOSE:
 *
 *
 *****************************************************************************/
// Vertex type used by the primitives batcher
struct sDrawQueuePrimitive
{
    D3DPRIMITIVETYPE              type;
    std::vector<PrimitiveVertice> vertices;
};
//
// Batches primitives drawing
//
class CPrimitiveBatcher
{
public:
    ZERO_ON_NEW
    CPrimitiveBatcher(bool m_bZTest);
    ~CPrimitiveBatcher(void);
    void OnDeviceCreate(IDirect3DDevice9* pDevice, float fViewportSizeX, float fViewportSizeY);
    void OnChangingRenderTarget(uint uiNewViewportSizeX, uint uiNewViewportSizeY);
    void UpdateMatrices(float fViewportSizeX, float fViewportSizeY);
    void Flush(void);
    void DrawPrimitive(D3DPRIMITIVETYPE eType, size_t iSize, const void* pDataAddr, size_t iVertexStride);
    void ClearQueue(void);
    void AddPrimitive(sDrawQueuePrimitive primitive);

protected:
    bool                             m_bZTest;
    IDirect3DDevice9*                m_pDevice;
    std::vector<sDrawQueuePrimitive> m_primitiveList;
    float                            m_fViewportSizeX;
    float                            m_fViewportSizeY;
    D3DXMATRIX                       m_MatView;
    D3DXMATRIX                       m_MatProjection;
};

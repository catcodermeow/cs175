#ifndef __MAC__
#include <GL/glew.h>
#endif

#include "picker.h"

using namespace std;

Picker::Picker(const RigTForm &initialRbt, const ShaderState &curSS)
    : drawer_(initialRbt, curSS), idCounter_(0),
      srgbFrameBuffer_(true) {}

bool Picker::visit(SgTransformNode &node) {
    // add to stack
    nodeStack_.push_back(node.shared_from_this());
    return drawer_.visit(node);
}

bool Picker::postVisit(SgTransformNode &node) {
    // pop from stack
    nodeStack_.pop_back();
    return drawer_.postVisit(node);
}

bool Picker::visit(SgShapeNode &node) {
    // increase the id counter, find the SgRbtNode that is closest from the top of the stack (which should be the shape node’s parent), and add the association between the id counter and the SgRbtNode to the map.
    idCounter_ ++;
    shared_ptr<SgRbtNode> parent;
    for (auto it = nodeStack_.rbegin(); it != nodeStack_.rend(); it--) {
        shared_ptr<SgRbtNode> q = dynamic_pointer_cast<SgRbtNode>(*it);
        if (q != nullptr) {
            parent = q;
            break;
        }
    }
    addToMap(idCounter_, parent);
//  convert the ID to RGB color, and pass it to the uniform variable uIdColor through the handle h uIdColor in ShaderState. You can query the drawer for the current ShaderState by calling its getCurSS() method
    const Cvec3 idcolor = idToColor(idCounter_);
    safe_glUniform3f(drawer_.getCurSS().h_uIdColor, idcolor[0], idcolor[1], idcolor[2]);
    return drawer_.visit(node);
}

bool Picker::postVisit(SgShapeNode &node) {
    // TODO
    // nothing other than drawer?
    return drawer_.postVisit(node);
}

shared_ptr<SgRbtNode> Picker::getRbtNodeAtXY(int x, int y) {
    PackedPixel p;
    glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &p);
    int id = colorToId(p);
    return find(id);
//    return shared_ptr<SgRbtNode>(); // return null for now
}

//------------------
// Helper functions
//------------------
//
void Picker::addToMap(int id, shared_ptr<SgRbtNode> node) {
    idToRbtNode_[id] = node;
}

shared_ptr<SgRbtNode> Picker::find(int id) {
    IdToRbtNodeMap::iterator it = idToRbtNode_.find(id);
    if (it != idToRbtNode_.end())
        return it->second;
    else
        return shared_ptr<SgRbtNode>(); // set to null
}

// encode 2^4 = 16 IDs in each of R, G, B channel, for a total of 16^3 number of
// objects...
static const int NBITS = 4, N = 1 << NBITS, MASK = N - 1;

Cvec3 Picker::idToColor(int id) {
    assert(id > 0 && id < N * N * N);
    Cvec3 framebufferColor =
        Cvec3(id & MASK, (id >> NBITS) & MASK, (id >> (NBITS + NBITS)) & MASK);
    framebufferColor = framebufferColor / N + Cvec3(0.5 / N);

    if (!srgbFrameBuffer_)
        return framebufferColor;
    else {
        // if GL3 is used, the framebuffer will be in SRGB format, and the color
        // we supply needs to be in linear space...
        Cvec3 linearColor;
        for (int i = 0; i < 3; ++i) {
            linearColor[i] =
                framebufferColor[i] <= 0.04045
                    ? framebufferColor[i] / 12.92
                    : pow((framebufferColor[i] + 0.055) / 1.055, 2.4);
        }
        return linearColor;
    }
}

int Picker::colorToId(const PackedPixel &p) {
    const int UNUSED_BITS = 8 - NBITS;
    int id = p.r >> UNUSED_BITS;
    id |= ((p.g >> UNUSED_BITS) << NBITS);
    id |= ((p.b >> UNUSED_BITS) << (NBITS + NBITS));
    return id;
}

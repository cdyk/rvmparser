#include <cassert>
#include <chrono>
#include "Store.h"
#include "LinAlgOps.h"

namespace {

  struct QueueItem
  {
    Geometry* from;
    Connection* connection;
    Vec3f upWorld;
  };


  struct Context
  {
    Buffer<QueueItem> queue;
    Logger logger = nullptr;
    Store* store = nullptr;
    unsigned front = 0;
    unsigned back = 0;
    unsigned connectedComponents = 0;
    unsigned circularConnections = 0;
    unsigned connections = 0;

  };

  void enqueue(Context& context, Geometry* from, Connection* connection, const Vec3f& upWorld)
  {
    connection->temp = 1;

    assert(context.back < context.connections);
    context.queue[context.back].from = from;
    context.queue[context.back].connection = connection;
    context.queue[context.back].upWorld = upWorld;

    context.back++;
  }

  void handleCircularTorus(Context& context, Geometry* geo, unsigned offset, const Vec3f& upWorld)
  {
    const auto & M = geo->M_3x4;
    const auto N = Mat3f(M.data);
    const auto N_inv = inverse(N);
    auto & ct = geo->circularTorus;
    auto c = std::cos(ct.angle);
    auto s = std::sin(ct.angle);

    auto upLocal = normalize(mul(N_inv, upWorld));

    if (offset == 1) {
      // rotate back to xz
      upLocal = Vec3f(c*upLocal.x + s*upLocal.y,
                      -s*upLocal.x + c*upLocal.y,
                      upLocal.z);
    }
    geo->sampleStartAngle = std::atan2(upLocal.z, upLocal.x);
    if (!std::isfinite(geo->sampleStartAngle)) {
      geo->sampleStartAngle = 0.f;
    }

    auto ci = std::cos(geo->sampleStartAngle);
    auto si = std::sin(geo->sampleStartAngle);
    auto co = std::cos(ct.angle);
    auto so = std::sin(ct.angle);

    Vec3f upNew(ci, 0.f, si);

    Vec3f upNewWorld[2];
    upNewWorld[0] = mul(N, upNew);
    upNewWorld[1] = mul(N, Vec3f(c * upNew.x - s * upNew.y,
                                 s * upNew.x + c * upNew.y,
                                 upNew.z));

    if (true) {
      Vec3f p0(ct.radius * ci + ct.offset,
               0.f,
               ct.radius * si);

      Vec3f p1((ct.radius * ci + ct.offset) * co,
               (ct.radius * ci + ct.offset) * so,
               ct.radius * si);


      auto a0 = mul(geo->M_3x4, p0);
      auto b0 = a0 + 1.5f*ct.radius*upNewWorld[0];

      auto a1 = mul(geo->M_3x4, p1);
      auto b1 = a1 + 1.5f*ct.radius*upNewWorld[1];

      //if (context.front == 1) {
      //  if (geo->connections[0]) context.store->addDebugLine(a0.data, b0.data, 0x00ffff);
      //  if (geo->connections[1]) context.store->addDebugLine(a1.data, b1.data, 0x00ff88);
      //}
      //else if (offset == 0) {
      //  if (geo->connections[0]) context.store->addDebugLine(a0.data, b0.data, 0x0000ff);
      //  if (geo->connections[1]) context.store->addDebugLine(a1.data, b1.data, 0x000088);
      //}
      //else {
      //  if (geo->connections[0]) context.store->addDebugLine(a0.data, b0.data, 0x000088);
      //  if (geo->connections[1]) context.store->addDebugLine(a1.data, b1.data, 0x0000ff);
      //}
    }

    for (unsigned k = 0; k < 2; k++) {
      auto * con = geo->connections[k];
      if (con && !con->hasFlag(Connection::Flags::HasRectangularSide) && con->temp == 0) {
        enqueue(context, geo, con, upNewWorld[k]);
      }
    }


  }


  void handleCylinderSnoutAndDish(Context& context, Geometry* geo, unsigned offset, const Vec3f& upWorld)
  {
    auto M_inv = inverse(Mat3f(geo->M_3x4.data));

    auto upn = normalize(upWorld);

    auto upLocal = mul(M_inv, upn);
    upLocal.z = 0.f;  // project to xy-plane

    geo->sampleStartAngle = std::atan2(upLocal.y, upLocal.x);
    if (!std::isfinite(geo->sampleStartAngle)) {
      geo->sampleStartAngle = 0.f;
    }

    Vec3f upNewWorld = mul(Mat3f(geo->M_3x4.data), Vec3f(std::cos(geo->sampleStartAngle),
                                                         std::sin(geo->sampleStartAngle),
                                                         0.f));

    for (unsigned k = 0; k < 2; k++) {
      auto * con = geo->connections[k];
      if (con && !con->hasFlag(Connection::Flags::HasRectangularSide) && con->temp == 0) {
        enqueue(context, geo, con, upNewWorld);
      }
    }
  }

  void processItem(Context& context)
  {
    auto & item = context.queue[context.front++];

    for (unsigned i = 0; i < 2; i++) {
      if (item.from != item.connection->geo[i]) {
        auto * geo = item.connection->geo[i];
        switch (geo->kind) {

        case Geometry::Kind::Pyramid:
        case Geometry::Kind::Box:
        case Geometry::Kind::RectangularTorus:
        case Geometry::Kind::Sphere:
        case Geometry::Kind::Line:
        case Geometry::Kind::FacetGroup:
          assert(false && "Got geometry with non-circular intersection.");
          break;

        case Geometry::Kind::Snout:
        case Geometry::Kind::EllipticalDish:
        case Geometry::Kind::SphericalDish:
        case Geometry::Kind::Cylinder:
          handleCylinderSnoutAndDish(context, geo, item.connection->offset[i], item.upWorld);
          break;

        case Geometry::Kind::CircularTorus:
          handleCircularTorus(context, geo, item.connection->offset[i], item.upWorld);
          break;

        default:
          assert(false && "Illegal kind");
          break;
        }
      }
    }
  }

}

void align(Store* store, Logger logger)
{
  Context context;
  context.logger = logger;
  context.store = store;
  auto time0 = std::chrono::high_resolution_clock::now();
  for (auto * connection = store->getFirstConnection(); connection != nullptr; connection = connection->next) {
    connection->temp = 0;

    if (connection->flags == Connection::Flags::HasCircularSide) {
      context.circularConnections++;
    }
    context.connections++;
  }


  context.queue.accommodate(context.connections);
  for (auto * connection = store->getFirstConnection(); connection != nullptr; connection = connection->next) {
    if (connection->temp || connection->hasFlag(Connection::Flags::HasRectangularSide)) continue;

    // Create an arbitrary vector in plane of intersection as seed.
    const auto & d = connection->d;
    Vec3f b;
    if (std::abs(d.x) > std::abs(d.y) && std::abs(d.x) > std::abs(d.z)) {
      b = Vec3f(0.f, 1.f, 0.f);
    }
    else {
      b = Vec3f(1.f, 0.f, 0.f);
    }

    auto upWorld = normalize(cross(d, b));
    assert(std::isfinite(lengthSquared(upWorld)));

    context.front = 0;
    context.back = 0;
    enqueue(context, nullptr, connection, upWorld);
    do {
      processItem(context);
    } while (context.front < context.back);

    context.connectedComponents++;
  }
  auto time1 = std::chrono::high_resolution_clock::now();
  auto e0 = std::chrono::duration_cast<std::chrono::milliseconds>((time1 - time0)).count();

  logger(0, "%d connected components in %d circular connections (%lldms).", context.connectedComponents, context.circularConnections, e0);
}
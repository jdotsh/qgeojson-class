#include "qgeojson_p.h"
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsonarray.h>
#include <qgeocoordinate.h>
#include <qgeocircle.h>
#include <qgeopath.h>
#include <qgeopolygon.h>
#include <qdebug.h>
QT_BEGIN_NAMESPACE

/*! \class QGeoJson
    \inmodule Qt.labs.location
    \ingroup json
    \ingroup shared
    \reentrant
    \since WIP

    \brief The QGeoJson class provides a way to import and export a GeoJSON documents.

    QGeoJson is a class that read GeoJSON document using the toJsonDocument methods and can import and
    export this document in/from a QVariantMap build up like follows:

    The class has 2 public methods:

    QVariantMap importGeoJson(const QJsonDocument &geojsonDoc);
    QJsonDocument exportGeoJson(const QVariantMap &geojsonMap);
    the QvariantMap returned by the importer has a very specific architecture, shaped on the GeoJSON structure. The exporter accepts a QVariantMap with the same structure
    This QVariantMap always has at least one (key, value) pair.
    The key is QString containing one of the 9 GeoJson object types, which are:

    7 Geometry Object Types:
    - “Point”
    - “MultiPoint”
    - “LineString”
    - “MultiLineString”
    - “Polygon”
    - “MultiPolygon”
    - “GeometryCollection”

    2 Non-Geometry GeoJSON Object Types:
    - “Feature”
    - “FeatureCollection”

    The value crresponding to this kye has a different format depending on the type:

    Single Type Geometry Objects:
    Key: “Point”
    The value contains a QGeoCircle, The coordinates of the original GeoJSON point are stored in the center of this QGeoCircle

    Key: “LineString”
    The value contains a QGeoPath, The coordinates of the GeoJSON points included in the original LineString are stored in the path of the QGeoPath.

    Key: “Polygon”
    The value contains a QGeoPolygon, The coordinates of the GeoJSON points representing the outer perimeter are stored in the path of the QGeoPolygon, if holes are present, the coordinates are stored in the QGeoPolygon using the proper methods.

    Homogeneously Typed Multipart Geometry Objects:
    Key: “MultiPoint”
    The value contains a QList<QGeoCircle>, The coordinates of the original GeoJSON points are stored in the center of these QGeoCircles

    Key: “MultiLineString”
    The value contains a QList<QGeoPath>, each structured like explained in the LineString section.

    Key: “MultiPolygon”
    The value contains a QList<QGeoPolygon>, each QgeoPolygon is structured like explained in the Polygon section.

    Heterogeneous composition of the other geometry types
    Key: “GeometryCollection”
    The value contains a QVariantList, each element of this list is a QVariantMap corresponding to one of the above listed GeoJson Geometry types, plus the GeometryCollection type itself, albeit GeoJson RFC advises against nesting GeometryCollections.

    Objects including geometries and attributes for an entity
    Key: “Feature”
    The value contains a QVariantMap, This map has a (key, value) pair corresponding the the pair of QVariantMap originated from one of the 7 GeoJSON geometry types already described. One more (key, value) pair conains a “properties” key and an additional QVariantMap as value. The (key, value) pairs of this last properties Map are pairs of Qstrings

    Key: “FeatureCollection”
    The value contains a QList<QVariantMap>, each element of this list is a “Feature” type QVariantMap

    A GeoJSON document can be converted from its text-based representation to a QJVariantMap
    using GeoJson::importGeoJson().

    Validity of the parsed document can be queried with !isNull() or using external API's.
*/

static QGeoCoordinate importPointCoordinates(const QVariant &obtainedCoordinates)
{
    QGeoCoordinate parsedCoordinates;
    QVariantList list = obtainedCoordinates.value<QVariantList>();

    QVariantList::iterator iter; // iterating Point coordinates arrays
    int i = 0;
    for (iter = list.begin(); iter != list.end(); ++iter) {
        QString elementType;
        elementType.fromUtf8(iter->typeName()); // Not working with QStringLiteral
        switch (i) {
        case 0:
            parsedCoordinates.setLatitude(iter->toDouble());
            break;
        case 1:
            parsedCoordinates.setLongitude(iter->toDouble());
            break;
        case 2:
            parsedCoordinates.setAltitude(iter->toDouble());
            break;}
        i++;
    }
    return parsedCoordinates;
}

static QList<QGeoCoordinate> importLineStringCoordinates(const QVariant &obtainedCoordinates)
{
    QList <QGeoCoordinate> parsedCoordinatesLine;
    QVariantList list2 = obtainedCoordinates.value<QVariantList>();
    QGeoCoordinate pointForLine;

    QVariantList::iterator iter; // iterating the LineString coordinates nasted arrays
    for (iter = list2.begin(); iter != list2.end(); ++iter) {
        pointForLine = importPointCoordinates(*iter);
        parsedCoordinatesLine.append(pointForLine); // populating the QList of coordinates
    }
    return parsedCoordinatesLine;
}

static QList<QList<QGeoCoordinate>> importPolygonCoordinates(const QVariant &obtainedCoordinates)
{
    QList<QList<QGeoCoordinate>> parsedCoordinatesPoly;
    QVariantList obtainedCoordinatesList = obtainedCoordinates.value<QVariantList>();

    QList<QGeoCoordinate> coordinatesList; // iterating the Polygon coordinates nasted arrays
    for (const QVariant &coordinatesVariant: obtainedCoordinatesList) {
        coordinatesList = importLineStringCoordinates(coordinatesVariant);
        parsedCoordinatesPoly << coordinatesList;
    }
    return parsedCoordinatesPoly;
}

static QGeoCircle importPoint(const QVariantMap &pointMap)
{
    QGeoCircle parsedPoint;

    QString keyCoord = QStringLiteral("coordinates");

    QGeoCoordinate center;
    QVariant valueCoords = pointMap.value(keyCoord); // returns the value associated with the key coordinates (Point)
    center = importPointCoordinates(valueCoords);
    parsedPoint.setCenter(center);
    return parsedPoint;
}

static QGeoPath importLineString(const QVariantMap &lineMap)
{
    QGeoPath parsedLineString;

    QString keyCoord = QStringLiteral("coordinates");

    QList <QGeoCoordinate> coordinatesList;
    QVariant valueCoordinates = lineMap.value(keyCoord); // returns the value associated with the key coordinates (LineString)
    coordinatesList = importLineStringCoordinates(valueCoordinates); // import an array of QGeoCoordinate from a nested GeoJSON array
    parsedLineString.setPath(coordinatesList);
    return parsedLineString;
}

static QGeoPolygon importPolygon(const QVariantMap &polyMap)
{
    QGeoPolygon parsedPolygon;

    int i = 0; // meant to bypass the lack of iterator position tracking

    QString keyCoord = QStringLiteral("coordinates");

    QList<QList<QGeoCoordinate>> perimeters;
    QList<QList<QGeoCoordinate>> ::iterator iter;
    QVariant valueCoordinates = polyMap.value(keyCoord); // returns the value associated with the key coordinates (Polygon)
    perimeters = importPolygonCoordinates(valueCoordinates); // import an array of QList<QGeocoordinates>

    for (iter = perimeters.begin(); iter != perimeters.end(); ++iter) {
        if (i == 0)
            parsedPolygon.setPath(*iter); // external perimeter
        else
            parsedPolygon.addHole(*iter); // inner perimeters
        i++;
    }
    return parsedPolygon;
}

static QVariantList importMultiPoint(const QVariantMap &multiPointMap)
{
    QVariantList parsedMultiPoint;

    QString keyCoord = QStringLiteral("coordinates");

    QGeoCircle parsedPoint;
    QGeoCoordinate coordinatesCenter;

    QVariant listCoords = multiPointMap.value(keyCoord);
    QVariantList list = listCoords.value<QVariantList>();
    QVariantList::iterator iter; // iterating MultiPoint coordinates nasted arrays
    for (iter = list.begin(); iter != list.end(); ++iter) {
        coordinatesCenter = importPointCoordinates(*iter);
        parsedPoint.setCenter(coordinatesCenter);
        parsedMultiPoint.append(QVariant::fromValue(parsedPoint)); // adding the newly created QGeoCircle to the dastination QVariantList
    }
    return parsedMultiPoint;
}

static QVariantList importMultiLineString(const QVariantMap &multiLineStringMap)
{
    QVariantList parsedMultiLineString;
    QList <QGeoCoordinate> coordinatesList;
    QGeoPath parsedLineString;

    QString keyCoord = QStringLiteral("coordinates");


    QVariant listCoords = multiLineStringMap.value(keyCoord);
    QVariantList list = listCoords.value<QVariantList>();

    QVariantList::iterator iter; // iterating the MultiLineString coordinates nasted arrays using importLineStringCoordinates
    for (iter = list.begin(); iter != list.end(); ++iter) {
        coordinatesList = importLineStringCoordinates(*iter);
        parsedLineString.setPath(coordinatesList);
        parsedMultiLineString.append(QVariant::fromValue(parsedLineString));
    }
    return parsedMultiLineString;
}

static QVariantList importMultiPolygon(const QVariantMap &multiPolyMap)
{
    QVariantList parsedMultiPoly;
    QString keyCoord = QStringLiteral("coordinates");

    QList<QList<QGeoCoordinate>> coordinatesList;
    QList<QList<QGeoCoordinate>>::iterator iter;
    QGeoPolygon singlePoly;
    QVariant valueCoordinates = multiPolyMap.value(keyCoord);

    QVariantList list = valueCoordinates.value<QVariantList>();
    for (const QVariant &polyVariantCoords: list) {
        coordinatesList = importPolygonCoordinates(polyVariantCoords);
        for (iter = coordinatesList.begin(); iter != coordinatesList.end(); ++iter) {
            if (coordinatesList.indexOf(*iter) == 0)
                singlePoly.setPath(*iter);
            else
                singlePoly.addHole(*iter);
        }
        parsedMultiPoly << QVariant::fromValue(singlePoly);
    }
    return parsedMultiPoly;
}

static QVariantMap importGeometry(const QVariantMap &geometryMap);

static QVariantList importGeometryCollection(const QVariantMap &geometryCollection)
{
    QVariantList parsedGeoCollection;

    QString keyGeometries = QStringLiteral("geometries");
    QVariant listGeometries = geometryCollection.value(keyGeometries);
    QVariantList list = listGeometries.value<QVariantList>();

    QVariantList::iterator iterGeometries;
    for (iterGeometries = list.begin(); iterGeometries != list.end(); ++ iterGeometries) {
        QVariantMap geometryMap = iterGeometries->value<QVariantMap>();
        QVariantMap geoMap = importGeometry(geometryMap);
        parsedGeoCollection.append(geoMap);
    }
    return parsedGeoCollection;
}

static QVariantMap importGeometry(const QVariantMap &geometryMap)
{
    QVariantMap parsedGeoJsonMap;
    QString geometryTypes[] = {
        QStringLiteral("Point"),
        QStringLiteral("MultiPoint"),
        QStringLiteral("LineString"),
        QStringLiteral("MultiLineString"),
        QStringLiteral("Polygon"),
        QStringLiteral("MultiPolygon"),
        QStringLiteral("GeometryCollection")
    };

    enum geoTypeSwitch {
        Point,
        MultiPoint,
        LineString,
        MultiLineString,
        Polygon,
        MultiPolygon,
        GeometryCollection
    };

    const QString geoKey;
    const QString keyGeometries = QStringLiteral("geometries");
    QVariant geoValue = geometryMap.value(QStringLiteral("type"));

    int len = sizeof(geometryTypes)/sizeof(*geometryTypes);
    int i = 0;

    for (i = 0; i<len-1; i++) {
        if (geoValue == geometryTypes[i])
            break;
    }

    switch (i) {
    case Point:
    {
        const QString geoKey = QStringLiteral("Point");
        QGeoCircle circle = importPoint(geometryMap);
        QVariant geoValue = QVariant::fromValue(circle);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    case MultiPoint:
    {
        const QString geoKey = QStringLiteral("MultiPoint"); // creating the key for the first element of the QVariantMap that will be returned
        QVariantList multiCircle = importMultiPoint(geometryMap);
        QVariant geoValue = QVariant::fromValue(multiCircle); // wraps up the multiCircle item in a QVariant
        parsedGeoJsonMap.insert(geoKey, geoValue); // creating the QVariantMap element
        break;
    }
    case LineString:
    {
        const QString geoKey = QStringLiteral("LineString");
        QGeoPath lineString = importLineString(geometryMap);
        QVariant geoValue = QVariant::fromValue(lineString);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    case MultiLineString:
    {
        const QString geoKey = QStringLiteral("MultiLineString");
        QVariantList multiLineString = importMultiLineString(geometryMap);
        QVariant geoValue = QVariant::fromValue(multiLineString);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    case Polygon:
    {
        const QString geoKey = QStringLiteral("Polygon");
        QGeoPolygon poly = importPolygon(geometryMap);
        QVariant geoValue = QVariant::fromValue(poly);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    case MultiPolygon:
    {
        const QString geoKey = QStringLiteral("MultiPolygon");
        QVariantList multiPoly = importMultiPolygon(geometryMap);
        QVariant geoValue = QVariant::fromValue(multiPoly);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    case GeometryCollection: // list of GeoJson geometry objects
    {
        const QString geoKey = QStringLiteral("GeometryCollection");
        QVariantList multigeo = importGeometryCollection(geometryMap);
        QVariant geoValue = QVariant::fromValue(multigeo);
        parsedGeoJsonMap.insert(geoKey, geoValue);
        break;
    }
    }
    return parsedGeoJsonMap;
}

static QVariantMap importFeature(const QVariantMap &feature)
{
    QVariantMap parsedFeature;
    QString key = QStringLiteral("geometry");
    QVariant featureGeometry = feature.value(key); // Importing GeoJson "geometry" member from the QVariantMap

    QVariantMap mapGeometry = featureGeometry.value<QVariantMap>();
    QVariantMap geoMap = importGeometry(mapGeometry);

    QVariant variantValue = QVariant::fromValue(geoMap);
    parsedFeature.insert(key, variantValue);

    variantValue = feature.value(QStringLiteral("properties"));  // Importing GeoJson "properties" member from the QVariantMap
    key = QStringLiteral("properties");
    parsedFeature.insert(key, variantValue);

    variantValue = feature.value(QStringLiteral("id")); // Importing GeoJson "id" member from the QVariantMap
    if (variantValue != QVariant::Invalid) {
        key = QStringLiteral("id");
        parsedFeature.insert(key, variantValue);
    }
    return parsedFeature;
}

static QVariantList importFeatureCollection(const QVariantMap &featureCollection)
{
    QVariantList parsedFeatureCollection;
    QString keyFeatures = QStringLiteral("features");
    QVariant featureVariant = featureCollection.value(keyFeatures);
    QVariantList featureVariantList = featureVariant.value<QVariantList>();
    QString keyFeature = QStringLiteral("Feature");

    QVariantMap importedMap;
    for (const QVariant &singleVariantFeature: featureVariantList) {
        QVariantMap featureMap = singleVariantFeature.value<QVariantMap>();
        QVariantMap featMap = importFeature(featureMap);
        importedMap.insert(keyFeature,featMap);
        parsedFeatureCollection.append(importedMap);
    }
    return parsedFeatureCollection;
}

static QJsonValue exportPointCoordinates(const QGeoCoordinate &obtainedCoordinates)
{
    QJsonValue geoLat = obtainedCoordinates.latitude();
    QJsonValue geoLong = obtainedCoordinates.longitude();

    QJsonArray array = {geoLat, geoLong};
    QJsonValue geoAlt;

    if (!qIsNaN(obtainedCoordinates.altitude())) {
        geoAlt = obtainedCoordinates.altitude();
        array.append(geoAlt);
    }
    QJsonValue geoArray = array;
    return geoArray;
}

static QJsonValue exportLineStringCoordinates(const QList<QGeoCoordinate> &obtainedCoordinatesList)
{
    QJsonValue lineCoordinates;
    QJsonValue multiPosition;
    QJsonArray arrayPosition;

    for (const QGeoCoordinate &pointCoordinates: obtainedCoordinatesList) {
        multiPosition = exportPointCoordinates(pointCoordinates);
        arrayPosition.append(multiPosition);
    }
    lineCoordinates = arrayPosition;
    return lineCoordinates;
}

static QJsonValue exportPolygonCoordinates(const QList<QList<QGeoCoordinate>> &obtainedCoordinates)
{
    QJsonValue lineCoordinates;
    QJsonValue polyCoordinates;
    QJsonArray arrayPath;
    for (const QList<QGeoCoordinate> &parsedPath: obtainedCoordinates) {
        lineCoordinates = exportLineStringCoordinates(parsedPath);
        arrayPath.append(lineCoordinates);
    }
    polyCoordinates = arrayPath;
    return polyCoordinates;
}

static QJsonObject exportPoint(const QVariantMap &pointMap)
{
    QJsonObject parsedPoint;

    QString keyType = QStringLiteral("type");
    QString keyCoord = QStringLiteral("coordinates");
    QString valuePoint = QStringLiteral("Point");

    QVariant geoCircle = pointMap.value(valuePoint); // exporting the value of "Point" QVariantMap node in a QVariant
    QGeoCircle circle = geoCircle.value<QGeoCircle>();
    QGeoCoordinate center = circle.center();
    QJsonValue valueType = valuePoint;

    parsedPoint.insert(keyType,valueType);
    parsedPoint.insert(keyCoord, exportPointCoordinates(center));
    return parsedPoint;
}

static QJsonObject exportLineString(const QVariantMap &lineStringMap)
{
    QJsonObject parsedMultiPoint;

    QString keyType = QStringLiteral("type");
    QString keyCoord = QStringLiteral("coordinates");

    QString valueLineString = QStringLiteral("LineString");

    QJsonValue lineCoordinates;

    QList <QGeoCoordinate> lineCoordinatesList;
    QVariant pathVariant = lineStringMap.value(valueLineString);
    QJsonValue valueType = valueLineString;

    QGeoPath parsedPath;
    parsedPath = pathVariant.value<QGeoPath>();

    lineCoordinatesList = parsedPath.path();
    lineCoordinates = exportLineStringCoordinates(lineCoordinatesList);

    parsedMultiPoint.insert(keyType, valueType);
    parsedMultiPoint.insert(keyCoord, lineCoordinates);
    return parsedMultiPoint;
}

static QJsonObject exportPolygon(const QVariantMap &polygonMap)
{
    QJsonObject parsedPolygon;

    QString keyType = QStringLiteral("type");

    QString keyCoord = QStringLiteral("coordinates");

    QString valuePolygon = QStringLiteral("Polygon");
    QJsonValue polyCoordinates;

    QGeoPolygon parsedPoly;

    QList<QList<QGeoCoordinate>> obtainedCoordinatesPoly;
    QVariant polygonVariant = polygonMap.value(valuePolygon);

    QJsonValue valueType = valuePolygon;

    parsedPoly = polygonVariant.value<QGeoPolygon>();
    obtainedCoordinatesPoly << parsedPoly.path();

    if (parsedPoly.holesCount()!=0)
        for (int i = 0; i<parsedPoly.holesCount(); i++) {
            obtainedCoordinatesPoly << parsedPoly.holePath(i);
        }
    polyCoordinates = exportPolygonCoordinates(obtainedCoordinatesPoly);
    qDebug() << " 8: " << parsedPolygon;
    parsedPolygon.insert(keyType, valueType);
    qDebug() << " 9: " << parsedPolygon;
    parsedPolygon.insert(keyCoord, polyCoordinates);
    qDebug() << " 10: " << parsedPolygon;
    return parsedPolygon;
}

static QJsonObject exportMultiPoint(const QVariantMap &multiPointMap)
{
    QJsonObject parsedMultiPoint;

    QString keyType = QStringLiteral("type");
    QString keyCoord = QStringLiteral("coordinates");

    QString valueMultiPoint = QStringLiteral("MultiPoint");

    QJsonValue multiPosition;

    QList <QGeoCoordinate> obtainedCoordinatesMP;
    QVariant multiCircleVariant = multiPointMap.value(valueMultiPoint);
    QVariantList multiCircleVariantList = multiCircleVariant.value<QVariantList>();
    QJsonValue typeValue = valueMultiPoint;

    for (const QVariant &exCircle: multiCircleVariantList) {
        obtainedCoordinatesMP << exCircle.value<QGeoCircle>().center();
    }
    multiPosition = exportLineStringCoordinates(obtainedCoordinatesMP);

    parsedMultiPoint.insert(keyType, typeValue);
    parsedMultiPoint.insert(keyCoord, multiPosition);
    return parsedMultiPoint;
}

static QJsonObject exportMultiLineString(const QVariantMap &multiLineStringMap)
{
    QJsonObject parsedMultiLineString;

    QString keyType = QStringLiteral("type");
    QString keyCoord = QStringLiteral("coordinates");
    QString valueMultiLineString = QStringLiteral("MultiLineString");

    QList<QList<QGeoCoordinate>> obtainedCoordinatesMLS;
    QVariant multiPathVariant = multiLineStringMap.value(valueMultiLineString);
    QVariantList multiPathList = multiPathVariant.value<QVariantList>();
    QJsonValue typeValue = valueMultiLineString;

    for (const QVariant &singlePath: multiPathList ){
        obtainedCoordinatesMLS << singlePath.value<QGeoPath>().path();
    }

    parsedMultiLineString.insert(keyType, typeValue);
    parsedMultiLineString.insert(keyCoord, exportPolygonCoordinates(obtainedCoordinatesMLS));
    return parsedMultiLineString;
}

static QJsonObject exportMultiPolygon(const QVariantMap &multiPolygonMap)
{
    QJsonObject parsedMultiPolygon;

    QString keyType = QStringLiteral("type");
    QString keyCoord = QStringLiteral("coordinates");
    QString valueMultiPolygon = QStringLiteral("MultiPolygon");

    QJsonValue polyCoordinates;

    QJsonArray parsedArrayPolygon;
    QList<QList<QGeoCoordinate>> obtainedPolyCoordinates;

    QVariant multiPolygonVariant = multiPolygonMap.value(valueMultiPolygon);
    QVariantList multiPolygonList = multiPolygonVariant.value<QVariantList>();

    QJsonValue typeValue = valueMultiPolygon;
    int polyHoles = 0;
    int currentHole;

    for (const QVariant &singlePoly: multiPolygonList ) { // Start parsing èolygon list
        obtainedPolyCoordinates << singlePoly.value<QGeoPolygon>().path(); // Extract external polygon path
        polyHoles = singlePoly.value<QGeoPolygon>().holesCount();

        if (polyHoles) //Check if the polygon has holes
            for (currentHole = 0 ; currentHole<polyHoles; currentHole++)
                obtainedPolyCoordinates << singlePoly.value<QGeoPolygon>().holePath(currentHole);
        polyCoordinates = exportPolygonCoordinates(obtainedPolyCoordinates); //Generates QJsonDocument compatible value
        parsedArrayPolygon.append(polyCoordinates); // Adds one level of nesting in coordinates
        obtainedPolyCoordinates.clear(); // Clears the temporary polygon linear ring storage
    }
    QJsonValue parsed = parsedArrayPolygon;

    parsedMultiPolygon.insert(keyType, typeValue);
    parsedMultiPolygon.insert(keyCoord, parsed);
    return parsedMultiPolygon;
}

static QJsonObject exportGeometry(const QVariantMap &geometryMap);

static QJsonObject exportGeometryCollection(const QVariantMap &geometryCollection)
{
    QString keyType = QStringLiteral("type");
    QString keyGeometries = QStringLiteral("geometries");
    QString valueGeometryCollection = QStringLiteral("GeometryCollection");

    QJsonObject parsed;
    QJsonObject parsedGeometry;
    QVariantList geometriesList;

    QJsonValue valueGeometries;
    QJsonArray parsedGeometries;

    geometriesList = geometryCollection.value(valueGeometryCollection).value<QVariantList>();
    for (const QVariant &extractedGeometry: geometriesList){
        parsedGeometry = exportGeometry(extractedGeometry.value<QVariantMap>());
        valueGeometries = parsedGeometry;
        parsedGeometries.append(valueGeometries);
    }
    valueGeometries = parsedGeometries;
    QJsonValue valueType = valueGeometryCollection;

    parsed.insert(keyType, valueType);
    parsed.insert(keyGeometries, valueGeometries);
    return parsed;
}

static QJsonObject exportGeometry(const QVariantMap &geometryMap)
{
    QJsonObject newObject;
    QJsonDocument newDocument;
    if (geometryMap.contains(QStringLiteral("Point"))) // check if the map contains the key Point
        newObject = exportPoint(geometryMap);
    if (geometryMap.contains(QStringLiteral("MultiPoint"))) // check if the map contains the key MultiPoint
        newObject = exportMultiPoint(geometryMap);
    if (geometryMap.contains(QStringLiteral("LineString")))
        newObject = exportLineString(geometryMap);
    if (geometryMap.contains(QStringLiteral("MultiLineString")))
        newObject = exportMultiLineString(geometryMap);
        newDocument.setObject(newObject);
    if (geometryMap.contains(QStringLiteral("Polygon")))
        newObject = exportPolygon(geometryMap);
    if (geometryMap.contains(QStringLiteral("MultiPolygon")))
        newObject = exportMultiPolygon(geometryMap);
    if (geometryMap.contains(QStringLiteral("GeometryCollection")))
        newObject = exportGeometryCollection(geometryMap);
    return newObject;
}

static QJsonObject exportFeature(const QVariantMap &feature)
{
    QString keyType = QStringLiteral("type");
    QString keyFeature = QStringLiteral("geometry");
    QString keyProp = QStringLiteral("properties");
    QString keyId = QStringLiteral("id");
    QString valueFeat = QStringLiteral("Feature");

    QJsonObject parsedFeature;
    QJsonObject exportedFeature;
    QJsonObject exportedProp;
    QJsonObject valueProp;

    QVariantMap featureMap;
    QVariantMap feat;

    QJsonValue valueFeature;
    QJsonValue valueProperties;
    QJsonValue valueId;

    featureMap = feature.value(valueFeat).value<QVariantMap>();
    QJsonValue valueType = valueFeat;
    feat = featureMap.value(keyFeature).value<QVariantMap>();
    exportedFeature = exportGeometry(feat);
    valueFeature = exportedFeature;

    feat = featureMap.value(keyProp).value<QVariantMap>();

    parsedFeature.insert(keyType, valueType);
    parsedFeature.insert(keyFeature, valueFeature);

    valueProp = featureMap.value(keyProp).value<QVariant>().toJsonObject();
    valueProperties = valueProp;
    parsedFeature.insert(keyProp, valueProperties);

    valueId = featureMap.value(keyId).value<QVariant>().toJsonValue();
    parsedFeature.insert(keyId, valueId);
    return parsedFeature;
}

static QJsonObject exportFeatureCollection(const QVariantMap &featureCollection)
{
    QString keyType = QStringLiteral("type");
    QString keyFeature = QStringLiteral("features");
    QString valueFeat = QStringLiteral("FeatureCollection");

    QJsonObject parsedFeatureCollection;
    QJsonObject exportedFeature;

    QVariantMap featureMap;
    QVariantMap featfeat;

    QJsonValue valueType;
    QJsonArray array;

    QVariant extractedFeatureVariant = featureCollection.value(valueFeat);
    QVariantList extractedFeaturVariantList = extractedFeatureVariant.value<QVariantList>();
    qDebug() << " 11: " << valueFeat;
    QJsonValue valueFeature = valueFeat;



    for (const QVariant &singleVariantFeature: extractedFeaturVariantList) {
            featureMap = singleVariantFeature.value<QVariantMap>();
            exportedFeature = exportFeature(featureMap);
            valueFeature = exportedFeature;
            array.append(valueFeature);
    }
    valueFeature = array;
         qDebug() << " 13: " << keyType;
                  qDebug() << " 14: " << valueType;
    parsedFeatureCollection.insert(keyType, valueFeat);
         qDebug() << " 12: " << parsedFeatureCollection;
    parsedFeatureCollection.insert(keyFeature, valueFeature);
    return parsedFeatureCollection;
}

QVariantMap QGeoJson::importGeoJson(const QJsonDocument &importDoc)
{
    QJsonObject object = importDoc.object(); // Read json object from imported doc
    QVariantMap standardMap = object.toVariantMap(); // extraced map using Qt's API
    qDebug() << " 2: " << standardMap;

    QString geoType[] = {
        QStringLiteral("Point"),
        QStringLiteral("MultiPoint"),
        QStringLiteral("LineString"),
        QStringLiteral("MultiLineString"),
        QStringLiteral("Polygon"),
        QStringLiteral("MultiPolygon"),
        QStringLiteral("GeometryCollection"),
        QStringLiteral("Feature"),
        QStringLiteral("FeatureCollection")
    };

    enum geoTypeSwitch {
        Point,
        MultiPoint,
        LineString,
        MultiLineString,
        Polygon,
        MultiPolygon,
        GeometryCollection,
        Feature,
        FeatureCollection
    };

    // Checking wether the JSON object has a "type" member

    QString keyType = QStringLiteral("type");

    QVariant keyVariant = standardMap.value(keyType);
    qDebug() << " 3: " << keyVariant;
        if (keyVariant == QVariant::Invalid) {
             // [x] Type check failed
        }
    QString valueType = keyVariant.value<QString>();
    int len = sizeof(geoType)/sizeof(*geoType);

    int i = 0; // index of the iteration where the value of type has been veryfied

    // Checking whether the "type" member has a GeoJSON admitted value

    for (i=0; i<len-1; i++) {
        qDebug() << " 3a-" << i << geoType[i];
        qDebug() << " 3b-" << i << valueType;
        if (valueType == geoType[i])
            break;
        else if (i==len-1) {}
    }

    QVariantMap parsedGeoJsonMap;
    switch (i) {
    case Point:
    {
        QString keyMap = QStringLiteral("Point");
        QGeoCircle circle = importPoint(standardMap);
        QVariant valueMap = QVariant::fromValue(circle);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case MultiPoint:
    {
        QString keyMap = QStringLiteral("MultiPoint"); // creating the key for the first element of the QVariantMap that will be returned
        QVariantList multiCircle = importMultiPoint(standardMap);
        QVariant valueMap = QVariant::fromValue(multiCircle); // wraps up the multiCircle item in a QVariant
        QList <QGeoCircle> testlist;

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case LineString:
    {
        QString keyMap = QStringLiteral("LineString");
        QGeoPath lineString = importLineString(standardMap);
        QVariant valueMap = QVariant::fromValue(lineString);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case MultiLineString:
    {
        QString keyMap = QStringLiteral("MultiLineString");
        QVariantList multiLineString = importMultiLineString(standardMap);
        QVariant valueMap = QVariant::fromValue(multiLineString);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case Polygon:
    {
        QString keyMap = QStringLiteral("Polygon");
        QGeoPolygon poly = importPolygon(standardMap);
        QVariant valueMap = QVariant::fromValue(poly);
        qDebug() << "4:" << parsedGeoJsonMap;
        parsedGeoJsonMap.insert(keyMap, valueMap);

        break;
    }
    case MultiPolygon:
    {
        QString keyMap = QStringLiteral("MultiPolygon");
        QVariantList multiPoly = importMultiPolygon(standardMap);
        QVariant valueMap = QVariant::fromValue(multiPoly);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case GeometryCollection: // list of GeoJson geometry objects
    {
        QString keyMap = QStringLiteral("GeometryCollection");
        QVariantList multiGeo = importGeometryCollection(standardMap);
        QVariant valueMap = QVariant::fromValue(multiGeo);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case Feature: // single GeoJson geometry object with properties
    {
        QString keyMap = QStringLiteral("Feature");
        QVariantMap feat = importFeature(standardMap);
        QVariant valueMap = QVariant::fromValue(feat);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    case FeatureCollection: // heterogeneous list of GeoJSON geometries with properties
    {
        QString keyMap = QStringLiteral("FeatureCollection");
        QVariantList featCollection = importFeatureCollection(standardMap);
        QVariant valueMap = QVariant::fromValue(featCollection);

        parsedGeoJsonMap.insert(keyMap, valueMap);
        break;
    }
    }

    // searching for the bbox member, if found, copy it to the output QVariantMap
    QString keyMap = QStringLiteral("bbox");
    QVariant bboxValue = standardMap.value(keyMap);
    if (bboxValue != QVariant::Invalid) {
        parsedGeoJsonMap.insert(keyMap, bboxValue);
    }
    return parsedGeoJsonMap;
}

QJsonDocument QGeoJson::exportGeoJson(const QVariantMap &exportMap)
{
    qDebug() << " 5: " << exportMap;
    QJsonObject newObject;
    QJsonDocument newDocument;
    if (exportMap.contains(QStringLiteral("Point"))) // check if the map contains the key Point
        newObject = exportPoint(exportMap);
    if (exportMap.contains(QStringLiteral("MultiPoint")))
        newObject = exportMultiPoint(exportMap);
    if (exportMap.contains(QStringLiteral("LineString")))
        newObject = exportLineString(exportMap);
    if (exportMap.contains(QStringLiteral("MultiLineString")))
        newObject = exportMultiLineString(exportMap);
    if (exportMap.contains(QStringLiteral("Polygon"))){
        qDebug() << " 6: " << exportMap;
        newObject = exportPolygon(exportMap);
            qDebug() << " 7: " << newObject;}
    if (exportMap.contains(QStringLiteral("MultiPolygon")))
        newObject = exportMultiPolygon(exportMap);
    if (exportMap.contains(QStringLiteral("GeometryCollection")))
        newObject = exportGeometry(exportMap);
    if (exportMap.contains(QStringLiteral("Feature")))
        newObject = exportFeature(exportMap);
    if (exportMap.contains(QStringLiteral("FeatureCollection")))
        newObject = exportFeatureCollection(exportMap);
    newDocument.setObject(newObject);
    return newDocument;
}

QT_END_NAMESPACE
